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

from importer import ShpImporter

# Module to load TomTom road data (Multinet)
class MultinetImporter(ShpImporter):
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # Shapefile names to load, without the extension and prefix. It will be the table name.
    SHAPEFILES = ['nw', 'jc', 'mn', 'cf', '2r', 'rn', 'mp', 'is', 'ig', 'rs', 'td', 'sr', 'st']
    # SQL files to execute before loading shapefiles
    PRELOADSQL = ["reset_import_schema.sql"]
    # SQL files to execute after loading shapefiles
    POSTLOADSQL = ["multinet.sql"]

    SPEEDPROFILES = []

    def __init__(self, source = "", source_speed = "", prefix = "", dbstring = "", logfile = None,
                 options = {'g':'geom', 'D':True, 'I':True, 'S':True}, doclean = True, subs = {}):
        super(MultinetImporter, self).__init__(source, prefix, dbstring, logfile, options, doclean, subs)

        if isinstance(source_speed, list):
            for source in source_speed:
                print "Importing source {}".format(source)
                self.prefix = self.get_fileprefix(source, prefix)
                self.get_speed_profiles(source)
        elif source_speed is not None:
            self.prefix = self.get_fileprefix(source_speed, prefix)
            self.get_speed_profiles(source_speed)
            pass
        
    def get_fileprefix(self, source, prefix = ""):
        """Get prefix for shapefiles. If given prefix is empty, try to find it browsing the directory."""
        myprefix = ""
        if prefix:
            myprefix = prefix
        else:
            # prefix has not been given, try to deduce it from files
            if source:
                prefixes = []
                if not os.path.isdir(source):
                    print "{} n'est pas un dir".format(source)
                    return ''
                for filename in os.listdir(source):
                    for shp in self.SPEEDPROFILES:
                        # if we find the table name at the end of the file name (w/o ext), add prefix to the list
                        # only check dbf and shp
                        basename, ext = os.path.splitext(os.path.basename(filename))
                        if ext.lower() in ['.dbf', '.shp'] and basename[-len(shp):] == shp:
                            curprefix = basename[:-len(shp)]
                            # only consider prefixes with "_"
                            if '_' in curprefix and curprefix not in prefixes:
                                prefixes.append(curprefix)
                # if only one prefix found, use it !
                if len(prefixes) > 1:
                    sys.stderr.write("Cannot determine prefix, multiple found : %s \n" % ",".join(prefixes))
                elif len(prefixes) == 1:
                    return prefixes[0]
                else:
                    return ''
        return myprefix

    def get_speed_profiles(self, source):
        notfound = []

        baseDir = os.path.realpath(source)
        ls = os.listdir(baseDir)
        for shp in self.SPEEDPROFILES:
            filenameShp = self.prefix + shp + ".shp"
            filenameDbf = self.prefix + shp + ".dbf"
            lsLower = [x.lower() for x in ls]
            if filenameShp in lsLower:
                i = lsLower.index(filenameShp)
                self.shapefiles.append((shp, os.path.join(baseDir, ls[i])))
            elif filenameDbf in lsLower:
                i = lsLower.index(filenameDbf)
                # only need one table speed profile HSPR
                if shp != 'hspr' or len(filter(lambda sp: sp[0] == 'hspr', self.shapefiles)) == 0: 
                    self.shapefiles.append((shp, os.path.join(baseDir, ls[i])))
            else:
                notfound.append(filenameDbf)
                sys.stderr.write("Warning : file for table %s not found.\n"\
                                     "%s not found\n" % (shp, filenameDbf))
        return notfound


# Module to load TomTom road data (Multinet)
class MultinetImporter1409(MultinetImporter):
    """This class enables to load TomTom Multinet data into a PostGIS database."""
    # SQL files to execute after loading shapefiles and speed profiles
    POSTLOADSQL = ["multinet1409.sql"]
    
    SPEEDPROFILES = ['hsnp','hspr']


