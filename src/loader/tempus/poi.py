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

    def __init__(self, source = "", prefix = "", dbstring = "", logfile = None,
            options = {'g':'geom', 'D':True, 'I':True, 'S':True}, doclean = True, poi_type = 5):
        ShpImporter.__init__(self, source, prefix, dbstring, logfile, options, doclean)
        if poi_type == 5:
            self.POSTLOADSQL = ['poi.sql']
        elif poi_type == 4:
            self.POSTLOADSQL = ['poi_scycle.sql']
        elif poi_type == 3:
            self.POSTLOADSQL = ['poi_cycle.sql']
        elif poi_type == 2:
            self.POSTLOADSQL = ['poi_carpoint.sql']
        elif poi_type == 1:
            self.POSTLOADSQL = ['poi_carpark.sql']

    def check_input(self):
        """Check if input is ok : given shape file exists."""
        ret = False
        if os.path.isfile(self.source):
            ret = True
        return ret

    def get_shapefiles(self):
        self.shapefiles = [self.source]
        return
    
