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
class MultinetImporter:
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['nw', 'jc', 'mn', 'cf', '2r', 'rn', 'mp', 'is', 'ig', 'cf', 'rs', 'td', 'sr'] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def __init__(self, source_dir = "", prefix = "", schema_out = "", dbstring = "", logfile = None):
        self.shapefiles = []
        self.source_dir = source_dir
        self.prefix = prefix
        self.get_shapefiles()
        self.dbstring = dbstring
        self.logfile = logfile
        self.sloader = ShpLoader(dbstring = dbstring, schema = "_tempus_import",
                logfile = self.logfile, options = {'I':True})

    def check_input(self):
        return len(MultinetImporter.SHAPEFILES) == len(self.shapefiles)

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
        return self.load_sqlfiles(MultinetImporter.PRELOADSQL)

    def postload_sql(self):
        return self.load_sqlfiles(MultinetImporter.POSTLOADSQL)

    def load_sqlfiles(self, files):
        ploader = PsqlLoader(dbstring = self.dbstring, logfile = self.logfile)
        ret = True
        for sqlfile in files:
            filename = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'sql', sqlfile)
            # Stop if one SQL execution was wrong
            if ret and os.path.isfile(filename):
                ploader.set_sqlfile(filename)
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
                self.sloader.set_table(MultinetImporter.SHAPEFILES[i])
                ret = self.sloader.load()
        return ret


    def set_dbparams(self, dbstring = ""):
        self.dbstring = dbstring
        self.sloader.set_dbparams(dbstring)

    def get_shapefiles(self):
        self.shapefiles = []
        notfound = []
        for shp in MultinetImporter.SHAPEFILES:
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

