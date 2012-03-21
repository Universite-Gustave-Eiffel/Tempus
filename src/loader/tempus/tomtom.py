#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os

from tools import ShpLoader
from dbtools import PsqlLoader

# Module to load TomTom road data (Multinet)
class MultinetLoader:
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['nw', 'jc', 'mn', 'cf', '2r']
    # SQL files to execute before loading shapefiles
    PRELOADSQL = []
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def __init__(self, source_dir = "", prefix = "", schema_out = "", dbstring = ""):
        self.shapefiles = []
        self.source_dir = source_dir
        self.prefix = prefix
        self.get_shapefiles()
        self.dbstring = dbstring
        self.sloader = ShpLoader(dbstring = dbstring, schema = schema_out,
                options = {'I':True})

    def check_input(self):
        return len(MultinetLoader.SHAPEFILES) == len(self.shapefiles)

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
        return self.load_sqlfiles(MultinetLoader.PRELOADSQL)

    def postload_sql(self):
        return self.load_sqlfiles(MultinetLoader.POSTLOADSQL)

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
                self.sloader.set_table(MultinetLoader.SHAPEFILES[i])
                ret = self.sloader.load()
        return ret


    def set_dbparams(self, dbstring = ""):
        self.sloader.set_dbparams(dbstring)

    def get_shapefiles(self):
        self.shapefiles = []
        notfound = []
        for shp in MultinetLoader.SHAPEFILES:
            filename = os.path.join(os.path.realpath(self.source_dir), self.prefix + shp + ".shp")
            if os.path.isfile(filename):
                self.shapefiles.append(filename)
            else:
                filename = os.path.join(os.path.realpath(self.source_dir), self.prefix + shp + ".dbf")
                if os.path.isfile(filename):
                    self.shapefiles.append(filename)
                else:
                    notfound.append(shp)
        return notfound

