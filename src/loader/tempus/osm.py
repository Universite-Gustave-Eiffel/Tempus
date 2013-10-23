#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os

from tools import ShpLoader
from dbtools import PsqlLoader
from importer import ShpImporter

# Module to load OpenStreetMap road data (OSM as shapefile)
class OSMImporter(ShpImporter):
    """This class enables to load OpenStreetMap Shape data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['highway'] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql", "tempus.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = ["osm.sql", "osm_processing.sql"]



