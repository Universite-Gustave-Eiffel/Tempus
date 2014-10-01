Some usage examples of the data loader:

./load_tempus -s ../../data/multinet -t tomtom -S 27572 -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.log

./load_tempus -s ../../data/rennes_gtfs.zip -t gtfs -S 4326 -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.log

./load_tempus -s ../../data/navstreet/ -t navteq -d "host=localhost dbname=tempus user=postgres port=5433"  -l /tmp/tempus.log

./load_tempus -s ../../data/point.shp -t poi -d "host=localhost dbname=tempus user=postgres port=5433"  -l /tmp/tempus.log

./load_tempus -s ../../data/multinet -t osm -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.load

OSM2SHP
-------

The Tempus loader for OSM data has been originally designed to work with OSM shapefiles exports from CloudMade.
Unfortunately this shapefile provider no longer delivers shapefile extracts.

The small 'osm2shp' utility then allows to convert a native OSM file to a shapefile readable by the Tempus loader.

OSM native file formats are:
* .osm, xml based file format
* .pbf, protobuffer based binary format

osm2shp only accepts .osm XML format in input.
Conversion from/to .osm and .pbf can be done through the use of utilities like osmosis.

OSM dumps in native formats may be obtained from different providers.
Have a look at http://wiki.openstreetmap.org/wiki/Planet.osm for a list of such mirrors.
Planet dumps can be downloaded as well as country / region extracts.

For instance, to import the road network from an OSM extract (pbf) into Tempus, use a command like :
osmosis --read-pbf /path/to/osm-extract.pbf --write-xml - | osm2shp -d . /dev/stdin

* "--write-xml -" is used to direct the pbf/osm conversion to the standard output
* using "/dev/stdin" (if you're using bash) allows to read data from the standard input

Public transportation data
--------------------------

The loader is able to import GTFS data and connect it to the road network.

GTFS data must be in EPSG:4326 coordinates (lat/lon).

The GTFS loader has been tested with the following datasets:
- Nantes public transport (bus, tramway) (TAN) : OK
- Loire Atlantique transports (bus) (LiLa) : OK
- Rennes public transport (bus, metro) (STAR) : some stops coordinates were using "," as decimal point. OK when replaced with "."
- Bordeaux public transport Data (tramway, bus) : OK
- transGironde transports (bus) : FAILURE : some stop points present in stop_times.txt are not defined in stops.txt
- Toulouse public transport (bus, tramway, metro) : OK
- TER trains from SNCF : OK
- Intercités trains from SNCF : OK
- Transilien train from SNCF : OK
- RATP : FAILURE - data contained coordinates expressed in radians, not degrees !

The GTFS loader must have underlying "road" nodes and sections to attach public transport nodes to.

Every ID found in a GTFS dataset are remapped to database IDs. The original id is kept within the 'vendor_id' field.
This remapping allows to append many GTFS datasets on a road networks.

Example
-------

Example of data loading on the city of Nantes (road network from OSM and public transport GTFS),
this is how the test database tempus_test_db is built :

It consists of :
- an import of Open Street Map on the area of Nantes, CC-BY-SA license.

(extract from CloudMade shapefiles Europe/France/"pays_de_la_loire")
./load_tempus -t osm -s /xxxx/ -p nantes_ -d "dbname=tempus_test_db"

- a cleanup of the road network (see src/loader/tempus/sql/road_cleaning.sql)

- the Z coordinate on road nodes and sections come from DEM data from SRTM

wget http://srtm.csi.cgiar.org/SRT-ZIP/SRTM_V41/SRTM_Data_GeoTiff/srtm_36_03.zip
unzip srtm_36_03.zip
gdalwarp -s_srs EPSG:4326 -t_srs EPSG:2154 srtm_36_03.tif srtm_36_03_2154.tif
raster2pgsql -I -t 20x20 -s 2154 srtm_36_03_2154.tif | psql tempus_test_db
(see src/loader/tempus/sql/dem_elevation.sql)

- an import of GTFS data from TAN provided by Nantes open data ("Arrêts, horaires et circuits TAN"), OdBL license

./load_tempus -t gtfs -s /xxxx/ARRETS_HORAIRES_CIRCUITS_TAN_GTFS.zip -d "dbname=tempus_test_db"

- an import of shared cycles points (Bicloo) provided by Nantes open data ("Liste des équipements publics (thème Mobilité)"), OdBL license

Shared bikes
./load_tempus -t poi -y 4 -v name:NOM_COMPLE service_name:Bicloo filter:type=100301 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154
Shared cars
./load_tempus -t poi -y 2 -v name:NOM_COMPLE service_name:Marguerite filter:type=100302 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154
Car parkings
/load_tempus -t poi -y 1 -v name:NOM_COMPLE filter:categorie=1001 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154

- two dummy turn restrictions on the road network (see the tempus.road_road table)

insert into tempus.road_restriction values (1, array[45727, 14590] );
insert into tempus.road_restriction values (2, array[45049,14228,45084] );
insert into tempus.road_restriction values (3, array[45049, 45049] );
# 60: for all types of cars
# 4 : car only (not taxi or car pool)
insert into tempus.road_restriction_time_penalty values (1, 0, 60, 'Infinity'::float);
insert into tempus.road_restriction_time_penalty values (2, 0, 4, 'Infinity'::float);
insert into tempus.road_restriction_time_penalty values (3, 0, 60, 'Infinity'::float);

-- some PT frequencies
-- Bus 26
insert into tempus.pt_frequency values (1, 25963, '08:00', '20:00', 5*60);
-- Tram 2
insert into tempus.pt_frequency values (2, 24689, '08:00', '20:00', 5*60);
