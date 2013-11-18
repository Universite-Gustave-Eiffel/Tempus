tempus_test_db.sql contains a database used for testing purpose.

It consists of :
- an import of Open Street Map on the area of Nantes, CC-BY-SA license.

(extract from CloudMade shapefiles "pays_de_la_loire")
./load_tempus -t osm -s /xxxx/ -p nantes_ -d "dbname=tempus_test_db"

- the Z coordinate on road nodes and sections come from DEM data from SRTM

raster2pgsql -I -t 20x20 -s 2154 ~/data/dem/srtm_36_03_2154.tif | psql tempus_test_db
(see src/loader/tempus/sql/dem_elevation.sql)

- an import of GTFS data from TAN provided by Nantes open data ("Arrêts, horaires et circuits TAN"), OdBL license

./load_tempus -t gtfs -s /xxxx/ARRETS_HORAIRES_CIRCUITS_TAN_GTFS.zip -d "dbname=tempus_test_db"

- an import of shared cycles points (Bicloo) provided by Nantes open data ("Liste des équipements publics (thème Mobilité)"), OdBL license

./load_tempus -t poi -y 4 -v name:NOM_COMPLE "filter:LIBTYPE='Bicloo'" parking_transport_type:128 -s /xxxx/LOC_EQUIPUB_MOBILITE_NM.shp -d "dbname=tempus_nantes" -W LATIN1 -S 2154

- two dummy turn restrictions on the road network (see the tempus.road_road table)

tempus_test_db=# select * from tempus.road_road;
 id | transport_types |    road_section     | road_cost 
----+-----------------+---------------------+-----------
  1 |            1031 | {45588,45053}       |        -1
  2 |            1031 | {44884,14023,44942} |        -1
