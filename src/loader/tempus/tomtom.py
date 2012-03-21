#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os

import tools
import dbtools

# Module to load TomTom road data (Multinet)
class MultinetLoader:
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # Shapefile names to load, without the extension. It will be the table name
    SHAPEFILES = ['nw', 'nl', 'jc', 'rn', 'nm', 'mp', '2r', 'is', 'ig', 'cf', 'rs', 'td', 'sr']
    # SQL files to execute before loading shapefiles
    PRELOADSQL = []
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def __init__(self, source_dir = "", schema_out = "", dbstring = ""):
        self.shapefiles = []
        self.set_source_dir(source_dir)
        self.sloader = tools.ShpLoader(dbstring = dbstring, schema = schema_out,
                options = {'I':True})

    def check_input(self):
        missing_file = False
        missing_filenames = []
        for shp in self.shapefiles:
            if not os.path.isfile(shp):
                missing_file = True
                missing_filenames.append(shp)
        return (not missing_file, missing_filenames)

    def load(self):
        ret = True
        if self.check_input():
            ret = self.preload_sql()
            if ret:
                ret = self.load_shapefiles()
            if ret:
                ret = self.postload_sql()
        return ret

    def preload_sql(self):
        return self.load_sqlfiles(PRELOADSQL)

    def postload_sql(self):
        return self.load_sqlfiles(POSTLOADSQL)

    def load_sqlfiles(self, files):
        ploader = PsqlLoader(dbstring = self.dbstring)
        ret = True
        for sqlfile in files:
            if ret and os.path.isfile(os.path.join(os.path.realpath(__file__), 'sql', sqlfile)):
                ploader.set_sqlfile(sqlfile)
                ret = ploader.load()
        return ret

    def load_shapefiles(self):
        """Load all given shapefiles into the database."""
        ret = True
        for i, shp in enumerate(self.shapefiles):
            # if one shapefile failed, stop there
            if ret:
                self.sloader.set_shapefile(shp)
                # the table name is the shapefile name without extension
                self.sloader.set_table(SHAPEFILE[i])
                ret = self.sloader.load()
        return ret


    def set_dbparams(self, dbstring = ""):
        self.sloader.set_dbparams(dbstring)

    def set_source_dir(self, source_dir):
        self.source_dir = source_dir
        self.shapefiles = [os.path.join(os.path.realpath(self.source_dir), shp + ".shp") for
            shp in SHAPEFILES]




