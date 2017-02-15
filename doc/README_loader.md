Some usage examples of the data loader:

```
loadtempus -s ../../data/multinet -t tomtom -S 27572 -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.log

loadtempus -s ../../data/rennes_gtfs.zip -t gtfs -S 4326 -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.log

loadtempus -s ../../data/navstreet/ -t navteq -d "host=localhost dbname=tempus user=postgres port=5433"  -l /tmp/tempus.log

loadtempus -s ../../data/point.shp -t poi -d "host=localhost dbname=tempus user=postgres port=5433"  -l /tmp/tempus.log

loadtempus -s ../../data/france-latest.osm.pbf -t osm -d "host=localhost dbname=tempus user=postgres port=5433" -l /tmp/tempus.load
```

OSM2Tempus
-------

The Tempus loader is able to load road data from the OpenStreetMap project.

It relies on a tool called `osm2tempus` that imports OSM PBF data efficiently. The `loadtempus` will automatically launch it when needed.

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
- [Nantes](nantes_data.md) (FIXME: obsolete)
- [Bordeaux](bordeaux_data.md) (FIXME: osbolete)

