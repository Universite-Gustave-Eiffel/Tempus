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
# Tempus data loader - OSM data source

import sys
import os
import subprocess
from dbtools import PsqlLoader

# Module to load OpenStreetMap road data (OSM as shapefile)
class OSMImporter:
    def __init__(self, source = "", dbstring = "", logfile = None):
        self.source = source
        self.dbstring = dbstring
        self.logfile = logfile

    def load(self):
        if self.logfile:
            try:
                out = open(self.logfile, "a")
                err = out
            except IOError as (errno, strerror):
                sys.stderr.write("%s : I/O error(%s): %s\n" % (self.logfile, errno, strerror))
        else:
            out = sys.stdout
            err = sys.stderr

        loader = PsqlLoader(dbstring = self.dbstring, logfile = self.logfile)
        # drop indexes before import
        loader.set_sqlfile(os.path.join(os.path.dirname(__file__), 'sql', 'drop_road_indexes.sql'))
        loader.load()
        
        command = ["osm2tempus"]
        command += ["-i", self.source]
        command += ["--pgis", self.dbstring]
        command += ["--nodes-table", "road_node"]
        p = subprocess.Popen(command, stdout = out, stderr = err)
        r = p.wait() == 0

        # recreate indexes after import
        loader.set_sqlfile(os.path.join(os.path.dirname(__file__), 'sql', 'create_road_indexes.sql'))
        loader.load()

        return r
        



