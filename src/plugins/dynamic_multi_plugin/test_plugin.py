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

script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../../python' )
sys.path.insert(0, wps_path)
from wps_client import *
from tempus_request import *

WPS_HOST = '127.0.0.1'
WPS_PATH = '/wps'


class TestWPS(unittest.TestCase):

    def setUp(self):
        self.client = HttpCgiConnection( WPS_HOST, WPS_PATH )
        self.wps = WPSClient(self.client)

    def test_plugin( self ):

        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        # we are requesting a U-turn, using a private car
        # where a restriction forbids U-turn to cars
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        origin = Point( vertex = 21906 ),
                        steps = [ RequestStep(destination = Point( vertex = 21864 )) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [4] # car
                        )
        # the resulting sequence should involve more sections
        self.assertEqual( len(tempus.results[0].steps), 8 )

        # we request the same U_turn, but with a bike
        # (allowed)
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        origin = Point( vertex = 21906 ),
                        steps = [ RequestStep(destination = Point( vertex = 21864 )) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [2] # bike
                        )
        self.assertEqual( len(tempus.results[0].steps), 5 )

        # we request the same U_turn, but walking
        # there is a shortcut in that case
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        origin = Point( vertex = 21906 ),
                        steps = [ RequestStep(destination = Point( vertex = 21864 )) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1] # pedestrian
                        )
        self.assertEqual( len(tempus.results[0].steps), 2 )

if __name__ == '__main__':
    unittest.main()


