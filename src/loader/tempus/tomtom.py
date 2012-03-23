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

# Module to load TomTom road data (Multinet)
class MultinetImporter(ShpImporter):
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['nw', 'jc', 'mn', 'cf', '2r', 'rn', 'mp', 'is', 'ig', 'cf', 'rs', 'td', 'sr'] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

