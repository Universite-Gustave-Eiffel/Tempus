#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os
import subprocess
from config import *

class PsqlLoader:
    def __init__(self, dbstring = "", sqlfile = "", replacements = {}):
        """Psql Loader constructor

        dbstring is a connexion string to a PostGIS database
            Ex : "dbname='databasename' host='addr' port='5432' user='x'"

        sqlfile is a text file containing SQL instructions

        replacements is a dictionnary containing values for key found in sqlfile to
            be replaced. Every occurence of "%key%" in the file will be replaced
            by the corresponding value in the dictionnary.
        """
        self.dbparams = self.extract_dbparams(dbstring) 
        self.sqlfile = sqlfile
        self.replatements = replacements

    def fill_template(self, template, values):
        """Takes a template text file and replace every occurence of
        "%key%" with the corresponding value in the given dictionnary."""
        raise NotImplementedError

    def set_sqlfile(self, sqlfile):
        self.sqlfile = sqlfile

    def set_from_template(self, template, values):
        """Set the SQL file by filling a template with values."""
        self.sqlfile = self.fill_template(template, values)

    def set_dbparams(self, dbstring):
        """Set database parameters."""
        self.dbparams = self.extract_dbparams(dbstring)

    def extract_dbparams(self, dbstring):
        """Get a dictionnary out of a classic dbstring."""
        ret = None
        if dbstring:
            ret = dict([(i, j.strip("' ")) for i, j in [i.split('=') for i in dbstring.split(' ')]])
        return ret

    def load(self):
        """Load SQL file into the DB."""
        res = False
        if self.dbparams and os.path.isfile(self.sqlfile):
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
            retcode = subprocess.call(command)
            if retcode == 0: res = True
        return res
