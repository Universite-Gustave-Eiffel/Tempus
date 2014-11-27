#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
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

import sys
import os

script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
plugins_path = os.path.abspath(script_path + '/../src/plugins')

if __name__ == '__main__':
    # look for a "test_plugin.py" script in each plugin subdirectory

    dirname, subdirs, _ = next(os.walk( plugins_path ))
    for d in subdirs:
        f = plugins_path + "/" + d + "/test_plugin.py"
        if os.path.isfile( f ):
            print "Testing " + d
            os.system(f)
