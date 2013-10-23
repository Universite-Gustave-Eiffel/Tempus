tempus_test_db.sql contains a database used for testing purpose.

It consists of :
- an import of Open Street Map on the area of Nantes, CC-BY-SA license.
- the 2 coordinate on road nodes and sections come from DEM data from SRTM
- an import of GTFS data from TAN provided by Nantes open data ("Arrêts, horaires et circuits TAN"), OdBL license
- an import of shared cycles points (Bicloo) provided by Nantes open data ("Liste des équipements publics (thème Mobilité)"), OdBL license

Nota : "stop_time" data have been removed from the db, since they represent more than 2.5M records and are not used yet.
