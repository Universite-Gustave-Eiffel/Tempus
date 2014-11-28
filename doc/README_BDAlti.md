Example of using DEM data
-------------------------

DEM data from [IGN BDAlti](http://professionnels.ign.fr/bdalti)

After creating a Tempus database with a road network, if you want to set the Z of each road node / section to the actual altitude based on a DEM layer,
proceed as follows :

* Load the DEM file into the database

Here for data around Nantes :

```
raster2pgsql -s 2154 -I -t 20x20 BDALTIr_2-0_MNT_EXT_0300_6750_LAMB93_IGN69_20110929.asc dem | psql tempus_bd
```

This will create a `dem` table in the database.

Now call the `dem_elevation.sql` script on it :

```
psql tempus_bd < src/loader/tempus/sql/dem_elevation.sql
```

You can now drop the raster table `dem`


