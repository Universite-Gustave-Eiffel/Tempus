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
import subprocess
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

    def test_reverse(self):
        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,16,24), type = 2 ), # after
                                              private_vehicule_at_destination = False) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1, 5] # walking, PT
        )
        s1 = list(tempus.results[0].steps)
        dt = tempus.results[0].starting_date_time
        start1 = dt.hour * 3600 + dt.minute * 60 + dt.second

        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'verbose_algo' : False, "verbose" : False },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,16,58), type = 1 ), # before
                                              private_vehicule_at_destination = False) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [1, 5] # walking, PT
        )
        s2 = list(tempus.results[0].steps)
        dt = tempus.results[0].starting_date_time
        start2 = dt.hour * 3600 + dt.minute * 60 + dt.second

        # check lengths
        self.assertEqual(len(s1), len(s2))

        for i, s in enumerate(s1):
            si = s1[i]
            sj = s2[i]
            self.assertEqual( si.__class__, si.__class__ )

        # check PT
        self.assertTrue( isinstance( s1[2], PublicTransportStep ) )
        self.assertTrue( isinstance( s2[2], PublicTransportStep ) )

        # check arrival time
        arrival1 = start1
        for s in s1:
            arrival1 += s.costs[Cost.Duration]*60
        arrival2 = start2
        for s in s2:
            arrival2 += s.costs[Cost.Duration]*60

        # FIXME why a difference of 1s ??
        self.assertTrue( (arrival1 - arrival2) ** 2 <= 1 )

    def test_speed_profiles(self):
        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        do_throw = False
        try:
            # test error: use profiles, but no time constraint
            tempus.request( plugin_name = 'dynamic_multi_plugin',
                            plugin_options = { 'Debug/verbose_algo' : False, "Debug/verbose" : False, "Time/use_speed_profiles" : True },
                            origin = Point( 350840.407710, 6688979.242950 ),
                            steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ), private_vehicule_at_destination = True) ],
                            criteria = [Cost.Duration],
                            allowed_transport_modes = [3] # private car
                        )
        except RuntimeError as e:
            do_throw = True
        self.assertTrue( do_throw )

        def get_duration( result ):
            dt = result.starting_date_time
            start1 = dt.hour * 3600 + dt.minute * 60 + dt.second
            arrival1 = start1
            for s in result.steps:
                arrival1 += s.costs[Cost.Duration]*60
            duration1 = (arrival1 - start1) / 60.0
            return duration1

        # without speed profiles
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'Debug/verbose_algo' : False, "Debug/verbose" : False },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,16,24), type = 2 ), # after
                                              private_vehicule_at_destination = True) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [3] # private car
        )
        d1 = get_duration( tempus.results[0] )
        self.assertTrue( (d1 - 20.7) ** 2 < 0.1)

        # with speed profiles, at 7 am
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'Debug/verbose_algo' : False, "Debug/verbose" : False, "Time/use_speed_profiles" : True },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,7,0), type = 2 ), # after @ 7:00
                                              private_vehicule_at_destination = True) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [3] # private car
        )
        d2 = get_duration( tempus.results[0] )
        self.assertTrue( (d2 - 34.8) ** 2 < 0.1 )

        # with speed profiles, at 12:05 am
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'Debug/verbose_algo' : False, "Debug/verbose" : False, "Time/use_speed_profiles" : True },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,12,5), type = 2 ), # after 12:05
                                              private_vehicule_at_destination = True) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [3] # private car
        )
        d3 = get_duration( tempus.results[0] )
        self.assertTrue( (d3 - 35.1) ** 2 < 0.1 )

        # check that if we set back to no profiles
        tempus.request( plugin_name = 'dynamic_multi_plugin',
                        plugin_options = { 'Debug/verbose_algo' : False, "Debug/verbose" : False, "Time/use_speed_profiles" : False },
                        origin = Point( 350840.407710, 6688979.242950 ),
                        steps = [ RequestStep(destination = Point( 355705.027259, 6688986.750080 ),
                                              constraint = Constraint( date_time = DateTime(2014,6,18,12,5), type = 2 ), # after 12:05
                                              private_vehicule_at_destination = True) ],
                        criteria = [Cost.Duration],
                        allowed_transport_modes = [3] # private car
        )
        d4 = get_duration( tempus.results[0] )
        self.assertTrue( (d4 - 20.7) ** 2 < 0.1 )

if __name__ == '__main__':
    unittest.main()


