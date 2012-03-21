#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

SHP2PGSQL="/usr/bin/shp2pgsql"
PSQL="/usr/bin/psql"

import os
import sys
import subprocess
import tempfile
from dbtools import PsqlLoader

class ShpLoader:
    """A static class to import shapefiles to PostgreSQL/PostGIS
    
    This is mainly a wrapper around the shp2pgsql tool"""
    def __init__(self, shapefile = "", dbstring = "", table = "", schema = "public", options = {}):
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
        self.ploader = PsqlLoader(dbstring)
        self.shapefile = shapefile
        # extract dbstring components
        self.table = table
        self.schema = schema
        self.options = options
        self.sqlfile = ""

    def shp2pgsql(self):
        """Generate a SQL file with shapefile content given specific options."""
        # check if shapefile exists
        if not os.path.isfile(self.shapefile):
            res = -1
        else:
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
            res = subprocess.call(command, stdout = tmpfile, stderr = sys.stderr) 
            tmpfile.close()
        return res

    def to_db(self):
        if self.ploader and os.path.isfile(self.sqlfile):
            self.ploader.set_sqlfile(self.sqlfile)
            self.ploader.load()

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

