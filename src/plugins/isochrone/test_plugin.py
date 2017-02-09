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
import re
import unittest

from wpstempus.wps_client import *
from wpstempus.tempus_request import *

WPS_HOST = '127.0.0.1'
WPS_PATH = '/wps'


class Test(unittest.TestCase):

    def setUp(self):
        self.client = HttpCgiConnection( WPS_HOST, WPS_PATH )
        self.wps = WPSClient(self.client)

    def test1( self ):
        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        limit = 50.0
        tempus.request( plugin_name = 'isochrone_plugin',
                        origin = Point( 356172.860489, 6687751.207350 ),
                        plugin_options = { 'Isochrone/limit' : limit },
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1] # pedestrian
                        )

        points = sorted(tempus.results[0].points, key = lambda x: x[3])
        for p in tempus.results[0].points:
            self.assertTrue(p[3] < limit)
        


if __name__ == '__main__':
    unittest.main()


