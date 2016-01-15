#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *   
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
"""

#
# Tempus data loader

import os
import sys
import subprocess
from config import *

class PsqlLoader:
    def __init__(self, dbstring = "", sqlfile = "", replacements = {}, logfile = None):
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
        self.replacements = replacements
        self.logfile = logfile

    def fill_template(self, template, values):
        """Takes a template text file and replace every occurence of
        "%(key)" with the corresponding value in the given dictionnary."""
        t = template
        for k, v in values.iteritems():
            t = t.replace( '%(' + k + ')', unicode(v) )
        return t;

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
        ret = {}
        if dbstring:
            ret = dict([(i, j.strip("' ")) for i, j in [i.split('=') for i in dbstring.split(' ')]])
        return ret

    def load(self):
        """Load SQL file into the DB."""
        res = False
        filename = ''
        if os.path.isfile(self.sqlfile):
            filename = self.sqlfile
            f = open( filename, 'r' )
            self.sqlfile = f.read()

        # call psql with sqlfile
        command = [PSQL]
        if self.dbparams.has_key('host'):
            command.append("--host=%s" % self.dbparams['host'])
        if self.dbparams.has_key('user'):
            command.append("--username=%s" % self.dbparams['user'])
        if self.dbparams.has_key('port'):
            command.append("--port=%s" % self.dbparams['port'])
        if self.dbparams.has_key('dbname'):
            command.append("--dbname=%s" % self.dbparams['dbname'])
        if self.logfile:
            try:
                out = open(self.logfile, "a")
                err = out
            except IOError as (errno, strerror):
                sys.stderr.write("%s : I/O error(%s): %s\n" % (self.logfile, errno, strerror))
        else:
            out = sys.stdout
            err = sys.stderr
        retcode = 0
        try:
            out.write("======= Executing SQL %s\n" % os.path.basename(filename) )
            out.flush()
            p = subprocess.Popen(command, stdin = subprocess.PIPE, stdout = out, stderr = err)
            self.sqlfile = "set client_min_messages=NOTICE;\n\\set ON_ERROR_STOP on\n" + self.sqlfile
            p.communicate( self.sqlfile )
            out.write("\n")
            retcode = p.returncode
        except OSError as (errno, strerror):
            sys.stderr.write("Error calling %s (%s) : %s \n" % (" ".join(command), errno, strerror))
        if self.logfile:
            out.close()
        return retcode == 0

def exec_sql(dbstring, query):
    proc = subprocess.Popen([PSQL, '-t', '-d', dbstring, '-c', query], stdout=subprocess.PIPE)
    (out, err) = proc.communicate()
    if err is not None:
        raise StandardError("PSQL error:" + err)
    return out
