#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import os
import zipfile
import tempfile
import csv
from tools import is_numeric
from importer import DataImporter

class GTFSImporter(DataImporter):
    """Public transportation GTFS data loader class."""
    GTFSFILES = [('agency', False),
            ('calendar', True),
            ('calendar_dates', True),
            ('fare_attributes', False),
            ('fare_rules', False),
            ('frequencies', False),
            ('routes', True),
            ('shapes', False),
            ('stop_times', True),
            ('stops', True),
            ('trips', True)]
    # SQL files to execute before loading GTFS data
    PRELOADSQL = ["create_gtfs_import_tables.sql"]
    # SQL files to execute after loading GTFS data 
    POSTLOADSQL = []

    def __init__(self, source = "", schema_out = "", dbstring = "", logfile = None):
        """Create a new GTFS data loader. Arguments are :
        source : a Zip file containing GTFS data
        schema_out : the destination schema in the database
        dbstring : the database connection string
        logfile : where to log SQL execution results (stdout by default)
        """
        super(GTFSImporter, self).__init__(source, schema_out, dbstring, logfile)
        self.sqlfile = ""

    def check_input(self):
        """Check if given source is a GTFS zip file."""
        res = False
        if zipfile.is_zipfile(self.source):
            res = True
            with zipfile.ZipFile(self.source) as zipf:
                filelist = zipf.namelist()
                for f, mandatory in GTFSImporter.GTFSFILES:
                    if res and "%s.txt" % f not in filelist:
                        if mandatory:
                            res = False
        return res

    def load_data(self):
        """Generate SQL file and load GTFS data to database."""
        self.sqlfile = self.generate_sql()
        self.load_gtfs()

    def generate_sql(self):
        """Generate a SQL file from GTFS feed."""
        sqlfile = ""
        if zipfile.is_zipfile(self.source):
            # create temp file for SQL output
            fd, sqlfile = tempfile.mkstemp()
            tmpfile = os.fdopen(fd, "w")
            # begin a transaction in SQL file
            tmpfile.write("BEGIN;\n")


            # open zip file
            with zipfile.ZipFile(self.source) as zipf:
                for f, mandatory in GTFSImporter.GTFSFILES:
                    # try to read the current GTFS txt file with CSV
                    try:
                        reader = csv.reader(zipf.open("%s.txt" % f),
                                delimiter = ',',
                                quotechar = '"')
                    # If we can't read and it's a mandatory file, error
                    except KeyError:
                        if mandatory:
                            raise ValueError, "Missing file in GTFS archive : %s" % f
                        else:
                            continue
                    # Write SQL for each beginning of table
                    tmpfile.write("-- Inserting values for table %\n\n" % f)
                    # first row is field names
                    fieldnames = reader.next()
                    # read the rows values
                    # deduce value type by testing
                    for row in reader:
                        insert_row = []
                        for value in row:
                            if value == '':
                              insert_row.append('NULL')
                            elif not is_numeric(value):
                              insert_row.append("'%s'" % value)
                            else:
                              insert_row.append(value)                           
                        # write SQL statement
                        tmpfile.write("INSERT INTO %s (%s) VALUES (%s);\n" %\
                                (f, ",".join(fieldnames), ','.join(insert_row)))
                    # Write SQL at end of processed table
                    tmpfile.write("-- Processed table %s.\n\n" % f)

            tmpfile.close()
        return sqlfile

    def load_gtfs(self):
        """Load generated SQL file with GTFS data into the database."""
        return self.load_sqlfiles(self.sqlfile)

    def clean(self):
        """Remove previously generated SQL file."""
        if os.path.isfile(self.sqlfile):
            os.remove(self.sqlfile)

    
