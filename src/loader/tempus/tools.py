SHP2PGSQL="/usr/bin/shp2pgsql"
PSQL="/usr/bin/psql"

import os
import sys
import subprocess
import tempfile

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
        self.shapefile = shapefile
        # extract dbstring components
        self.dbparams = self.extractDbParams(dbstring)
        self.table = table
        self.schema = schema
        self.options = options
        self.sqlfile = ""

    def shp2pgsql(self):
        """Generate a SQL file with shapefile content given specific options."""
        # check if shapefile exists
        # TODO

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

    def toDb(self):
        """Load previously generated SQL file into the DB."""
        if self.sqlfile and self.dbparams \
            and self.sqlfile and os.path.isfile(self.sqlfile):
            # call psql with sqlfile
            command = [PSQL]
            if self.dbparams.has_key('host'):
                command.append("--host=%s" % self.dbparams['host'])
            if self.dbparams.has_key('user'):
                command.append("--username=%s" % self.dbparams['user'])
            if self.dbparams.has_key('port'):
                command.append("--port=%s" % self.dbparams['port'])
            command.append("--file=%s" % self.sqlfile)
            if self.dbparams.has_key('dbname'):
                command.append("--dbname=%s" % self.dbparams['dbname'])
            res = subprocess.call(command)
            return res

    def clean(self):
        """Delete previously generated SQL file."""
        if os.path.isfile(self.sqlfile):
            os.remove(self.sqlfile)

    def extractDbParams(self, dbstring):
        """Get a dictionnary out of a classic dbstring."""
        ret = None
        if dbstring:
            ret = dict([(i, j.strip("' ")) for i, j in [i.split('=') for i in dbstring.split(' ')]])
        return ret

    # setters
    def setDbParams(self, dbstring):
        """Set database parameters."""
        self.dbparams = self.extractDbParams(dbstring)

    def setShapefile(self, filename):
        """Set shapefile name."""
        self.shapefile = filename

    def setTable(self, tablename):
        self.table = tablename

    def setSchema(self, schema):
        self.schema = schema

    def setOptions(self, options = {}):
        self.options = options

