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
from importer import ShpImporter

# Module to load a POI shape file
class POIImporter(ShpImporter):
    """This class enables to load POI data into a PostGIS database and link it to
    an existing network."""
    # Shapefile names to load, without the extension and prefix. 
    # Source shapefile set by user for this importer
    # this is used for table name
    SHAPEFILES = ["poi"] 
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles 
    POSTLOADSQL = []

    def __init__(self, source = "", prefix = "", dbstring = "", logfile = None,
            options = {'g':'geom', 'D':True, 'I':True, 'S':True}, doclean = True, poi_type = 5, subs = {}):
        if not 'name' in subs.keys():
            subs['name'] = 'pname'
        if not 'filter' in subs.keys():
            subs['filter'] = 'true'
        subs['poi_type'] = poi_type
        ShpImporter.__init__(self, source, prefix, dbstring, logfile, options, doclean, subs)

    def check_input(self):
        """Check if input is ok : given shape file exists."""
        if not os.path.isfile(self.source):
            raise StandardError("The given source file does not exist")

    def get_shapefiles(self):
        self.shapefiles = [("poi", self.source)]
        return
    
