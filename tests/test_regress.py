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

import sys
import os
import re
import unittest

script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../src/python' )
sys.path.insert(0, wps_path)
from wps_client import *
from tempus_request import *

WPS_HOST = '127.0.0.1'
WPS_PATH = '/wps'


class TestRegress(unittest.TestCase):

    def setUp(self):
        self.client = HttpCgiConnection( WPS_HOST, WPS_PATH )
        self.wps = WPSClient(self.client)

    def test_no_roadmap( self ):
        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )
        
        # same origin and destination
        ox = 354236.977275
        oy = 6691065.337142
        # must not crash
        tempus.request( plugin_name = 'sample_road_plugin',
                        origin = Point( ox, oy ),
                        steps = [ RequestStep(destination = Point(ox, oy)) ] )
                        
        # test no path
        tempus.request( plugin_name = 'sample_road_plugin',
                        origin = Point( 360854.282813, 6689475.272404 ),
                        steps = [ RequestStep(destination = Point(360939.067871, 6689392.909777)) ] )

if __name__ == '__main__':
    unittest.main()


