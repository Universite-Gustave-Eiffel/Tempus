Example of data loading on the city of Nantes (road network from OSM and public transport GTFS),
this is how the test database tempus_test_db is built :

It consists of :
- an import of Open Street Map on the area of Nantes, CC-BY-SA license.

(extract from CloudMade shapefiles Europe/France/"pays_de_la_loire")
```
./load_tempus -t osm -s /xxxx/ -p nantes_ -d "dbname=tempus_test_db"
```

- a cleanup of the road network (see [road_cleaning.sql](../src/loader/tempus/sql/road_cleaning.sql))

- the Z coordinate on road nodes and sections come from DEM data from SRTM

```
wget http://srtm.csi.cgiar.org/SRT-ZIP/SRTM_V41/SRTM_Data_GeoTiff/srtm_36_03.zip
unzip srtm_36_03.zip
gdalwarp -s_srs EPSG:4326 -t_srs EPSG:2154 srtm_36_03.tif srtm_36_03_2154.tif
raster2pgsql -I -t 20x20 -s 2154 srtm_36_03_2154.tif dem | psql tempus_test_db
```

(see [dem_elevation.sql](src/loader/tempus/sql/dem_elevation.sql))

(then drop table dem)

- an import of GTFS data from TAN provided by Nantes open data ("Arrêts, horaires et circuits TAN"), OdBL license

```
./load_tempus -t gtfs -v network:TAN -s /xxxx/ARRETS_HORAIRES_CIRCUITS_TAN_GTFS.zip -d "dbname=tempus_test_db"
```

- an import of shared cycles points (Bicloo) provided by Nantes open data ("Liste des équipements publics (thème Mobilité)"), OdBL license

Shared bikes

```
./load_tempus -t poi -y 4 -v name:NOM_COMPLE service_name:Bicloo filter:type=100301 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154
```

Shared cars

```
./load_tempus -t poi -y 2 -v name:NOM_COMPLE service_name:Marguerite filter:type=100302 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154
```

Car parkings

```
/load_tempus -t poi -y 1 -v name:NOM_COMPLE filter:categorie=1001 -s /xxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154
```

- two dummy turn restrictions on the road network (see the tempus.road_road table)

```
insert into tempus.road_restriction values (1, array[44986,11072] );
insert into tempus.road_restriction values (2, array[23272,12946,23931] );
insert into tempus.road_restriction values (3, array[23272,23272] );
-- 60: for all types of cars
-- 4 : car only (not taxi or car pool)
insert into tempus.road_restriction_time_penalty values (1, 0, 60, 'Infinity'::float);
insert into tempus.road_restriction_time_penalty values (2, 0, 4, 'Infinity'::float);
insert into tempus.road_restriction_time_penalty values (3, 0, 60, 'Infinity'::float);

-- some PT frequencies
-- Bus 26
insert into tempus.pt_frequency values (1, 25963, '08:00', '20:00', 5*60);
-- Tram 2
insert into tempus.pt_frequency values (2, 24689, '08:00', '20:00', 5*60);
```
