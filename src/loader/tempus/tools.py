#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence


import os
import sys
import subprocess
import tempfile
from dbtools import PsqlLoader
from config import *

def is_numeric(s):
    """Test if a value is of numeric type."""
    try:
      i = float(s)
    except ValueError:
        # not numeric
        return False
    else:
        # numeric
        return True

class ShpLoader:
    """A static class to import shapefiles to PostgreSQL/PostGIS
    
    This is mainly a wrapper around the shp2pgsql tool"""
    def __init__(self, shapefile = "", dbstring = "", table = "", schema = "public", logfile = None, options = {}, doclean = True):
        """Loader initialization
        
        shapefile is the name of the file to load
        dbstring is a connexion string to a PostGIS database
            Ex : "dbname='databasename' host='addr' port='5432' user='x'"
        table is the table name
        schema is the schema name
        options is a dictionnary of shp2pgsql options. Dict keys are the short name of
            shp2pgsql options (without the "-"), except for the "mode" key.
            "mode" has one value out of 'a', 'p', 'c', 'd'
        """
        self.ploader = PsqlLoader(dbstring, logfile = logfile)
        self.shapefile = shapefile
        # extract dbstring components
        self.table = table
        self.schema = schema
        self.options = options
        self.sqlfile = ""
        self.logfile = logfile
        self.doclean = doclean

    def load(self):
        """Generates SQL and load to database."""
        ret = False
        if self.shp2pgsql():
            if self.to_db():
                if self.doclean:
                    self.clean()
                ret = True
            else:
                sys.stderr.write("Database loading failed")
        else:
            sys.stderr.write("SQL generation failed")
        return ret
        # TODO : add better error reporting

    def shp2pgsql(self):
        """Generate a SQL file with shapefile content given specific options."""
        res = False
        # check if shapefile exists
        if not os.path.isfile(self.shapefile):
            res = False
        else:
            # if shapefile is only a DBF
            # then deactivate index and add "n" option
            if os.path.splitext(self.shapefile)[1] == ".dbf":
                self.options['n'] = True
                self.options['I'] = False

            # setup shp2pgsql command line
            command = [SHP2PGSQL]
            if self.options.has_key('s'):
                command.append("-s %s" % self.options['s'])
            if self.options.has_key("mode"):
                command.append("-%s" % self.options['mode'])
            if self.options.has_key("g"):
                command.append("-g %s" % self.options['g'])
            if self.options.has_key("D") and self.options['D'] is True:
                command.append("-D")
            if self.options.has_key("G") and self.options['G'] is True:
                command.append("-G")
            if self.options.has_key("k") and self.options['k'] is True:
                command.append("-k")
            if self.options.has_key("i") and self.options['i'] is True:
                command.append("-i")
            if self.options.has_key("I") and self.options['I'] is True:
                command.append("-I")
            if self.options.has_key("S") and self.options['S'] is True:
                command.append("-S")
            if self.options.has_key("w") and self.options['w'] is True:
                command.append("-w")
            if self.options.has_key("n") and self.options['n'] is True:
                command.append("-n")
            if self.options.has_key("W"):
                command.append("-W %s" % self.options['W'])
            if self.options.has_key("N"):
                command.append("-N %s" % self.options['N'])
            command.append("%s" % self.shapefile)
            target = ""
            if self.schema:
                target = "%s." % self.schema
            target += self.table
            command.append(target)

            # create temp file for SQL output
            fd, self.sqlfile = tempfile.mkstemp()
            tmpfile = os.fdopen(fd, "w")

            # call shp2pgsql
            if self.logfile:
                outerr = open(self.logfile, "a")
            else:
                outerr = sys.stderr
            rescode = subprocess.call(command, stdout = tmpfile, stderr = outerr) 
            if rescode == 0: res = True
            tmpfile.close()
            if self.logfile:
                outerr.close()
        return res

    def to_db(self):
        res = False
        if self.ploader and os.path.isfile(self.sqlfile):
            self.ploader.set_sqlfile(self.sqlfile)
            res = self.ploader.load()
        return res

    def clean(self):
        """Delete previously generated SQL file."""
        if os.path.isfile(self.sqlfile):
            os.remove(self.sqlfile)

    def set_dbparams(self, dbstring):
        """Set database parameters."""
        self.ploader.set_dbparams(dbstring)

    # setters
    def set_shapefile(self, filename):
        """Set shapefile name."""
        self.shapefile = filename

    def set_table(self, tablename):
        self.table = tablename

    def set_schema(self, schema):
        self.schema = schema

    def set_options(self, options = {}):
        self.options = options

