# How to load Grand Lyon Open Data (WIP

## OSM

Open Street map data of Rhones-Alpes (from http://download.geofabrik.de/europe/france/rhone-alpes.html)

```bash
wget http://download.geofabrik.de/europe/france/rhone-alpes-latest.osm.pbf
```

Cut it to Lyon agglo with osmosis (TODO check bounds. They are not too bad but...)

```bash
osmosis --read-pbf ../rhone-alpes-latest.osm.pbf --bounding-box  left=4.72 bottom=45.65 right=5.00 top=45.87 --write-pbf file=../lyon.osm.pbf
```

Insert it in DB

```bash
loadtempus -t osm -d "dbname=tempus_test_db" -R -s ../lyon.osm.pbf
```

## Lyon Open Data

### Velov

#### static

Download here: https://data.grandlyon.com/equipements/station-vflov/

Then execute:
```bash
loadtempus -t poi --poi-service-name "Station  velov" -y 4 -s ~/Documents/tempus/resultat-pvo_patrimoine_voirie.pvostationvelov/pvo_patrimoine_voirie.pvostationvelov.shp -d "dbname=tempus_test_db" -W LATIN1
```
**warning** this import seems cheesy: it stores the data into a \_tempus_import schema, not in the tempus schema. To be fixed.

#### real-time

https://data.grandlyon.com/equipements/station-vflov-disponibilitfs-temps-rfel/

### TCL

#### static datas

Download here:

https://data.grandlyon.com/equipements/rfseau-de-transport-en-commun-du-grand-lyon-tcl-sytral/
https://data.grandlyon.com/transport/horaires-thforiques-du-rfseau-tcl/

#### Real-time data

https://data.grandlyon.com/transport/prochains-passages-temps-rfel-du-rfseau-tcl/

### Other traffic informations

#### Real-time
- parkings: https://data.grandlyon.com/equipements/parcs-relais-du-rfseau-tcl-disponibilitfs-temps-rfel/
- traffic information: https://data.grandlyon.com/localisation/etat-du-trafic-temps-rfel/

