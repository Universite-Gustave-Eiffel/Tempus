Data
----

* Raw OSM http://download.geofabrik.de/europe/france/aquitaine-latest.osm.pbf
* bus GTFS http://data.lacub.fr/files.php?gid=68&format=14
* tram GTFS http://data.lacub.fr/files.php?gid=67&format=14
* équipements publics (shp L93) http://data.lacub.fr/files.php?gid=62&format=2
* stations VCUB (shp L93) http://data.lacub.fr/files.php?gid=43&format=2

Roads
-----

```
~/src/osmosis/bin/osmosis --read-pbf /data/osm/aquitaine/aquitaine-latest.osm.pbf --bb left=-0.8749 top=45.269 right=0.0505 bottom=44.3508 --wx - | bin/osm2shp -d /data/osm/aquitaine/tempus_shp /dev/stdin
# don't forget -R !
./load_tempus -t osm -s /data/osm/aquitaine/tempus_shp -S 4326 -d "dbname=tempus_bdx" -R
# clean the road network
psql tempus_bdx < loader/tempus/sql/road_cleaning.sql
```

Bus & Tram
----------

```
./load_tempus -t gtfs -v network:TBC -s /data/bordeaux/gtfs/keolis_bus.zip -d "dbname=tempus_bdx"
./load_tempus -t gtfs -v network:TBC -s /data/bordeaux/gtfs/keolis_tram.zip -d "dbname=tempus_bdx"
```

Parkings
--------

* Open TO_EQPUB_P.shp with QGIS
* Filter with "substr(SSTHEME,1,2) = 'J8' or substr(SSTHEME,1,3) = 'J10'" to select parkings

(If you have problems with float fields that have "," instead of ".", make sure to use o recent version (2.2) of shp2pgsql otherwise :
  - Remove column "geom_o", "toponyme_o", "toponyme_x", "toponyme_y"
  - Save selection to parkings.shp
)

```
./load_tempus -t poi -y 1 -v name:NOM -s /data/bordeaux/equipement_public/parkings.shp -S 2154 -d "dbname=tempus_bdx"
```

VLS
---

(Remove column "geom_o" from TB_STVEL_P.shp if you have problems with commas in floats)

```
./load_tempus -t poi -y 4 -v name:NOM service_name:VCUB -s /data/bordeaux/vls/TB_STVEL_P.shp -W LATIN1 -S 2154 -d "dbname=tempus_bdx"
```
