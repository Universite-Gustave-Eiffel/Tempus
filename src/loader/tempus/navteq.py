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

# Module to load Navteq road data (Navstreets)
class NavstreetsImporter(ShpImporter):
    """This class enables to load Navteq Navstreets data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['streets', 'cdms', 'rdms', 'altstreets']
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = ['navteq.sql']


