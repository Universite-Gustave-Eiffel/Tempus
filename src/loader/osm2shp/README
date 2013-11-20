
============================================================================

The osm2shp program in this directory has been swapped on 23 September 2010.
If you want the "old" osm2shp (which had last been modified in 2007), do

svn update -r 23322

The following is about the new osm2shp program which I'll dub the 
"Geofabrik osm2shp" because it has been (and is still) used to create the
free shapes on download.geofabrik.de.

============================================================================

This program converts OSM XML to ESRI Shape Files.

You need basic C programming skills if you want to make it create anything
else than the default set of shape files. The program consists of two parts;
one is the actual XML parsing and shapefile writing code, the other contains
the "logic" that determines which shapefiles get created, and what data is
copied to them.

This second file is called osm2shp.config. See comments in that file for 
details.

Note that this program creates UTF-8 shape files by default but you can specify
-DLATIN1 when compiling to switch to LATIN-1.

This program does not support relations and therefore no multipolygons either.

This progam is capable of producing very large shape files so be careful what 
you feed it. Unfortunately, neither reading from stdin nor reading compressed
or binary formats is supported so you really have to have a .osm file for input.

Written by Frederik Ramm <frederik@remote.org>, public domain

