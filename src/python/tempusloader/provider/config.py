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

# Configuration file for Tempus loader
# Replace following values with specific paths for your system

# import schema
IMPORTSCHEMA = "_tempus_import"

# Path where binaries are located (used below)
BINPATH=""

# PostgreSQL's console client binary location
PSQL=BINPATH + ""

# PostGIS shp2pgsql tool
SHP2PGSQL=BINPATH + ""

# DO NOT MODIFY UNDER THIS LINE
# Autoconfiguration attempts
import os

def is_exe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def which(program):
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

if not is_exe(PSQL):
    PSQL = which("psql")

if not is_exe(SHP2PGSQL):
    SHP2PGSQL = which("shp2pgsql")

if PSQL is None or SHP2PGSQL is None:
    raise OSError("Could not find psql and shp2pgsql.")
