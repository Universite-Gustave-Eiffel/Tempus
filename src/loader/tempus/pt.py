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
import zipfile
import tempfile
import csv
import sys
from tools import is_numeric
from importer import DataImporter
from config import *

class GTFSImporter(DataImporter):
    """Public transportation GTFS data loader class."""
    GTFSFILES = [('agency', False),
            ('calendar', True),
            ('calendar_dates', True),
            ('fare_attributes', False),
            ('fare_rules', False),
            ('frequencies', False),
            ('transfers', False),
            ('routes', True),
            ('shapes', False),
            ('stop_times', True),
            ('stops', True),
            ('trips', True)]
    # SQL files to execute before loading GTFS data
    PRELOADSQL = ["reset_import_schema.sql", "create_gtfs_import_tables.sql" ]
    # SQL files to execute after loading GTFS data 
    POSTLOADSQL = [ "gtfs.sql", "gtfs_shapes.sql" ]

    def __init__(self, source = "", dbstring = "", logfile = None, encoding = 'UTF8', copymode = True, doclean = True, subs = {}):
        """Create a new GTFS data loader. Arguments are :
        source : a Zip file containing GTFS data
        schema_out : the destination schema in the database
        dbstring : the database connection string
        logfile : where to log SQL execution results (stdout by default)
        """
        if not 'network' in subs.keys():
            subs['network'] = "PT"
        super(GTFSImporter, self).__init__(source, dbstring, logfile, doclean, subs)
        self.sqlfile = ""
        self.copymode = copymode
        self.doclean = doclean
        self.encoding = encoding

    def check_input(self):
        """Check if given source is a GTFS zip file."""
        if zipfile.is_zipfile(self.source):
            with zipfile.ZipFile(self.source) as zipf:
                filelist = [ os.path.basename(x) for x in zipf.namelist() ]
                for f, mandatory in GTFSImporter.GTFSFILES:
                    if mandatory and "%s.txt" % f not in filelist:
                        raise StandardError("Missing mandatory file: %s.txt" % f)
        else:
            raise StandardError("Not a zip file!")

    def load_data(self):
        """Generate SQL file and load GTFS data to database."""
        self.sqlfile = self.generate_sql()
        r = self.load_gtfs()
        if self.doclean:
            self.clean()
        return r

    def generate_sql(self):
        """Generate a SQL file from GTFS feed."""

        if self.logfile:
            out = open(self.logfile, "a")
        else:
            out = sys.stdout

        sqlfile = ""
        if zipfile.is_zipfile(self.source):
            # create temp file for SQL output
            fd, sqlfile = tempfile.mkstemp()
            tmpfile = os.fdopen(fd, "w")
            # begin a transaction in SQL file
            tmpfile.write("SET CLIENT_ENCODING TO %s;\n" % self.encoding)
            tmpfile.write("SET STANDARD_CONFORMING_STRINGS TO ON;\n")
            tmpfile.write("BEGIN;\n")


            # open zip file
            with zipfile.ZipFile(self.source) as zipf:

                # map of text file => (mandatory, zip_path)
                gFiles = {}
                for f, mandatory in GTFSImporter.GTFSFILES:
                    gFiles[f] = (mandatory, '')

                for zfile in zipf.namelist():
                    bn = os.path.basename( zfile )
                    for f, m in GTFSImporter.GTFSFILES:
                        if f + '.txt' == bn:
                            mandatory, p = gFiles[f]
                            gFiles[f] = ( mandatory, zfile )

                for f, v in gFiles.iteritems():
                    mandatory, f = v
                    if mandatory and f == '':
                        raise ValueError, "Missing file in GTFS archive : %s" % f

                for f, v in gFiles.iteritems():
                    mandatory, zpath = v
                    if zpath == '':
                        continue

                    out.write( "== Loading %s\n" % zpath )

                    # get rid of Unicode BOM (U+FEFF)
                    def csv_cleaner( f ):
                        for line in f:
                            yield line.replace('\xef\xbb\xbf', '')

                    reader = csv.reader(csv_cleaner(zipf.open( zpath )),
                                        delimiter = ',',
                                        quotechar = '"')

                    # Write SQL for each beginning of table
                    tmpfile.write("-- Inserting values for table %s\n\n" % f)
                    # first row is field names
                    fieldnames = reader.next()
                    if self.copymode:
                        tmpfile.write('COPY "%s"."%s" (%s) FROM stdin;\n' % (IMPORTSCHEMA, f, ",".join(fieldnames)))
                    # read the rows values
                    # deduce value type by testing
                    for row in reader:
                        insert_row = []
                        for value in row:
                            if value == '':
                                if self.copymode:
                                    insert_row.append('\N')
                                else:
                                    insert_row.append('NULL')
                            elif not self.copymode and not is_numeric(value):
                                insert_row.append("'%s'" % value.replace("'", "''"))
                            else:
                                insert_row.append(value)
                        # write SQL statement
                        if self.copymode:
                            tmpfile.write("%s\n" % '\t'.join(insert_row))
                        else:
                            tmpfile.write("INSERT INTO %s.%s (%s) VALUES (%s);\n" %\
                                    (IMPORTSCHEMA, f, ",".join(fieldnames), ','.join(insert_row)))
                    # Write SQL at end of processed table
                    if self.copymode:
                        tmpfile.write("\.\n")
                    tmpfile.write("\n-- Processed table %s.\n\n" % f)

            tmpfile.write("COMMIT;\n")
            tmpfile.write("-- Processed all data \n\n")
            tmpfile.close()
        return sqlfile

    def load_gtfs(self):
        """Load generated SQL file with GTFS data into the database."""
        return self.load_sqlfiles([self.sqlfile], substitute = False)

    def clean(self):
        """Remove previously generated SQL file."""
        if os.path.isfile(self.sqlfile):
            os.remove(self.sqlfile)

    
