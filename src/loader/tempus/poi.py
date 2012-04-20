#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os
from importer import ShpImporter

# Module to load a POI shape file
class POIImporter(ShpImporter):
    """This class enables to load POI data into a PostGIS database and link it to
    an existing network."""
    # Shapefile names to load, without the extension and prefix. 
    # Source shapefile set by user for this importer
    # this is used for table name
    SHAPEFILES = ["poi"] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def check_input(self):
        """Check if input is ok : given shape file exists."""
        ret = False
        if os.path.isfile(self.source):
            ret = True
        return ret

    def get_shapefiles(self):
        self.shapefiles = [self.source]
        return
    
