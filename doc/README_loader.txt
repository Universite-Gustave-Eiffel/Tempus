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

Some examples of loading public data (OSM and OpenData) on some cities are given in xxx_data.txt.
There are currently examples for:
- Nantes
- Bordeaux

