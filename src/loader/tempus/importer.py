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

    def __init__(self, source_dir = "", schema_out = "", dbstring = "", logfile = None):
        self.source_dir = source_dir
        self.dbstring = dbstring
        self.logfile = logfile
        self.sloader = ShpLoader(dbstring = dbstring, schema = IMPORTSCHEMA,
                logfile = self.logfile, options = {'I':True})

    def check_input(self):
        """Check if data input is ok."""
        raise True

    def import(self):
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
        ploader = PsqlLoader(dbstring = self.dbstring, logfile = self.logfile)
        ret = True
        for sqlfile in files:
            filename = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'sql', sqlfile)
            # Stop if one SQL execution was wrong
            if ret and os.path.isfile(filename):
                ploader.set_sqlfile(filename)
                ret = ploader.load()
        return ret

    def load_data(self):
        """Load the data into the database."""
        ret = True
        return ret

    def set_dbparams(self, dbstring = ""):
        self.dbstring = dbstring
        self.sloader.set_dbparams(dbstring)

