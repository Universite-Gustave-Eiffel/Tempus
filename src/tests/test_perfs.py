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
wps_path = os.path.abspath( script_path + '/../python' )
sys.path.insert(0, wps_path)
from wps_client import *
from tempus_request import *

WPS_HOST = '127.0.0.1'
WPS_PATH = '/wps'


class TestPerfs(unittest.TestCase):

    def setUp(self):
        self.client = HttpCgiConnection( WPS_HOST, WPS_PATH )
        self.wps = WPSClient(self.client)

    def test_perfs( self ):
        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        plugins = [p.name for p in tempus.plugin_list()]
        self.assertIn( 'sample_road_plugin', plugins, "sample_road_plugin is not loaded !")
        self.assertIn( 'sample_multi_plugin', plugins, "sample_multi_plugin is not loaded !")
        self.assertIn( 'dynamic_multi_plugin', plugins, "dynamic_multi_plugin is not loaded !")

        print "-- SAMPLE_ROAD - Across town - Walking --"
        tempus.request( plugin_name = 'sample_road_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False },
                        origin = Point( 355564.183649, 6683945.955172 ),
                        steps = [ RequestStep(destination = Point( 354318.103927, 6694572.246133 ), private_vehicule_at_destination = False) ],
                        criteria = [Cost.Distance],
                        allowed_transport_modes = [1] # walking
                        )
        iterations = int(tempus.metrics['iterations'])
        time_s = float(tempus.metrics['time_s'])
        us_per_it = time_s / iterations * 1000000.0
        print "Time: %.2fs\titerations: %d\tPer iteration: %.2fµs" % (time_s, iterations, us_per_it)

        print "-- SAMPLE_MULTI - Across town - Walking --"
        tempus.request( plugin_name = 'sample_multi_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False },
                        origin = Point( 355564.183649, 6683945.955172 ),
                        steps = [ RequestStep(destination = Point( 354318.103927, 6694572.246133 ), private_vehicule_at_destination = False) ],
                        criteria = [Cost.Distance],
                        allowed_transport_modes = [1] # walking
                        )
        iterations = int(tempus.metrics['iterations'])
        time_s = float(tempus.metrics['time_s'])
        us_per_it = time_s / iterations * 1000000.0
        print "Time: %.2fs\titerations: %d\tPer iteration: %.2fµs" % (time_s, iterations, us_per_it)

        print "-- DYNAMIC_MULTI - Dijkstra - Across town - Walking --"
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False },
                        origin = Point( 355564.183649, 6683945.955172 ),
                        steps = [ RequestStep(destination = Point( 354318.103927, 6694572.246133 ), private_vehicule_at_destination = False) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1] # walking
                        )
        iterations = int(tempus.metrics['iterations'])
        time_s = float(tempus.metrics['time_s'])
        us_per_it = time_s / iterations * 1000000.0
        print "Time: %.2fs\titerations: %d\tPer iteration: %.2fµs" % (time_s, iterations, us_per_it)

        print "-- DYNAMIC_MULTI - Dijkstra - Private car + parking --"
        # walking and private car, with a private parking on the path, but must park before reaching destination
        # car_park_search_time is artificially set to 0
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False, "car_parking_search_time" : 0 },
                        origin = Point( 355873.900102, 6687910.974614 ),
                        departure_constraint = Constraint( date_time = DateTime(2014,6,18,16,06) ),
                        steps = [ RequestStep(destination = Point( 354712.155537, 6688427.796341 ), private_vehicule_at_destination = False) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1, 3], # walking, private car
                        parking_location = Point(355745.798990, 6688016.989327)
                        )
        iterations = int(tempus.metrics['iterations'])
        time_s = float(tempus.metrics['time_s'])
        us_per_it = time_s / iterations * 1000000.0
        print "Time: %.2fs\titerations: %d\tPer iteration: %.2fµs" % (time_s, iterations, us_per_it)

if __name__ == '__main__':
    unittest.main()


