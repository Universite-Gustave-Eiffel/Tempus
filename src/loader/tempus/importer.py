#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os

from tools import ShpLoader
from dbtools import PsqlLoader
from config import *

# Base class for data importer
class DataImporter(object):
    """This class is a parent class which enable loading data to a PostgreSQL/PostGIS
    database. It loads nothing by default."""
    # SQL files to execute before loading data
    PRELOADSQL = []
    # SQL files to execute after loading data
    POSTLOADSQL = []

    def __init__(self, source = "", dbstring = "", logfile = None, doclean = True):
        self.source = source
        self.dbstring = dbstring
        self.logfile = logfile
        self.ploader = PsqlLoader(dbstring = self.dbstring, logfile = self.logfile)
        self.doclean = doclean

    def check_input(self):
        """Check if data input is ok."""
        raise True

    def clean(self):
        """Clean temporary files."""
        return True

    def load(self):
        ret = True
        if self.check_input():
            ret = self.preload_sql()
            if ret:
                ret = self.load_data()
            if ret:
                ret = self.postload_sql()
        return ret

    def preload_sql(self):
        return self.load_sqlfiles(self.PRELOADSQL)

    def postload_sql(self):
        return self.load_sqlfiles(self.POSTLOADSQL)

    def load_sqlfiles(self, files):
        """Load some SQL files to the defined database.
        Stop if one was wrong."""
        ret = True
        for sqlfile in files:
            filename = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'sql', sqlfile)
            # Stop if one SQL execution was wrong
            if ret and os.path.isfile(filename):
                self.ploader.set_sqlfile(filename)
                ret = self.ploader.load()
        return ret

    def load_data(self):
        """Load the data into the database."""
        ret = True
        return ret

    def set_dbparams(self, dbstring = ""):
        self.dbstring = dbstring
        self.ploader.set_dbparams(dbstring)


# Base class to import data from shape files
class ShpImporter(DataImporter):
    """This class enables to load shapefile data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = [] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = []
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def __init__(self, source = "", prefix = "", dbstring = "", logfile = None, options = {'D':True, 'I':True, 'S':True}, doclean = True):
        super(ShpImporter, self).__init__(source, dbstring, logfile)
        self.shapefiles = []
        self.prefix = prefix
        self.get_shapefiles()
        self.sloader = ShpLoader(dbstring = dbstring, schema = IMPORTSCHEMA,
                logfile = self.logfile, options = options)

    def check_input(self):
        """Check if data input is ok : we have the required number of shapefiles."""
        return len(self.SHAPEFILES) == len(self.shapefiles)

    def load_data(self):
        """Load all given shapefiles into the database."""
        ret = True
        for i, shp in enumerate(self.shapefiles):
            # if one shapefile failed, stop there
            if ret:
                self.sloader.set_shapefile(shp)
                # the table name is the shapefile name without extension
                self.sloader.set_table(self.SHAPEFILES[i])
                ret = self.sloader.load()
        return ret

    def set_dbparams(self, dbstring = ""):
        super(ShpImporter, self).set_dbparams(dbstring)
        self.sloader.set_dbparams(dbstring)

    def get_shapefiles(self):
        self.shapefiles = []
        notfound = []
        for shp in self.SHAPEFILES:
            filename = os.path.join(os.path.realpath(self.source), self.prefix + shp + ".shp")
            if os.path.isfile(filename):
                self.shapefiles.append(filename)
            else:
                filename = os.path.join(os.path.realpath(self.source), self.prefix + shp + ".dbf")
                if os.path.isfile(filename):
                    self.shapefiles.append(filename)
                else:
                    notfound.append(shp)
        return notfound

