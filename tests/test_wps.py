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


class TestWPS(unittest.TestCase):

    def setUp(self):
        self.client = HttpCgiConnection( WPS_HOST, WPS_PATH )
        self.wps = WPSClient(self.client)

    def test_connection(self):
        # Test null GET request
        [status, msg ] = self.client.request( 'GET', '')
        self.assertEqual( status, 400 )
        # Test no version
        [status, msg ] = self.client.request( 'GET', 'service=wps')
        self.assertEqual( status, 400 )
        # Test bad version
        [status, msg ] = self.client.request( 'GET', 'service=wps&version=1.2')
        self.assertEqual( status, 400 )
        # Test no operation
        [status, msg ] = self.client.request( 'GET', 'service=wps&version=1.0.0')
        [is_ex, code] = get_wps_exception( msg )
        self.assertEqual( status, 400 )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'OperationNotSupported' )
        # Test bad operation
        [status, msg ] = self.client.request( 'GET', 'service=wps&version=1.0.0&request=toto')
        [is_ex, code] = get_wps_exception( msg )
        self.assertEqual( status, 400 )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'OperationNotSupported' )
        # Test case sensivity
        [status, msg ] = self.client.request( 'GET', 'serVicE=wpS&veRsion=1.0.0&requEst=GetCapabilitIES')
        [ is_ex, code ] = get_wps_exception( msg )
        self.assertEqual( status, 400 )
        self.assertEqual( is_ex, True )
        # Test case sensivity
        [status, msg ] = self.client.request( 'GET', 'serVicE=wpS&veRsion=1.0.0&requEst=GetCapabilities')
        [ is_ex, code ] = get_wps_exception( msg )
        self.assertEqual( is_ex, False )

        # Test malformed XML input
        [status, msg ] = self.client.request( 'POST', '<xml' )
        [ is_ex, code ] = get_wps_exception( msg )
        self.assertEqual( status, 400 )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

        [status, msg ] = self.client.request( 'POST', '<Exe service="wps" version="1.0.0"><Identifier>id</Identifier></Exe>' )
        [is_ex, code] = get_wps_exception( msg )
        self.assertEqual( status, 400 )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'OperationNotSupported' )

        # Test invalid method
        [status, msg ] = self.client.request( 'POST', '<GetCapabilities service="wps" version="1.0.0"><Identifier>id</Identifier></GetCapabilities>' )
        self.assertEqual( status, 405 )

        # test bad version with POST
        [status, msg ] = self.client.request( 'POST', '<Execute service="wps" version="1.2.0"><Identifier>id</Identifier></Execute>' )
        self.assertEqual( status, 400 )

        # test bad service with POST
        [status, msg ] = self.client.request( 'POST', '<Execute service="dummy_service" version="1.0.0"><Identifier>id</Identifier></Execute>' )
        self.assertEqual( status, 400 )
        pass
    # Test error returns when service != 1.0.0

    def test_get_capabilities(self):
        [status, msg] =  self.wps.get_capabilities()
        self.assertEqual( status, 200 )
        
    def test_describe_process(self):
        [status, msg] = self.wps.describe_process( "select" )
        self.assertEqual( status, 200 )
        # TODO : test errors
        [status, msg] = self.wps.describe_process( "I do not exist" )
        self.assertEqual( status, 400 )
        [ is_exception, code ] = get_wps_exception( msg )
        self.assertEqual( is_exception, True )
        self.assertEqual( code, 'InvalidParameterValue' )
    
    def test_execute(self):
        # Test invalid identifier
        is_ex = False
        code = 0
        try:
            outputs = self.wps.execute( "I don't exist", {} )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

        # Test wrong number of input arguments
        is_ex = False
        code = 0
        try:
            # get_options_descriptions takes 1 argument
            outputs = self.wps.execute( "get_option_descriptions", { 'arg1' : ['arg1'], 'arg2' : ['arg2'] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )
            
        # TODO: test bad input argument name
        is_ex = False
        code = 0
        try:
            outputs = self.wps.execute( "get_option_descriptions", { 'plugi' : [ 'plugi'] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

        # Test non validating input
        is_ex = False
        code = 0
        try:
            # connect only takes 1 argument
            outputs = self.wps.execute( "get_option_descriptions", { 'plugin' : [ 'plugin', {'nam' : ''} ] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

    def test_execution_cycle(self):
        self.wps.execute( 'plugin_list', {} )

        # non existing plugin
        plugin_arg = { 'plugin': ['plugin', {'name' : 'I dont exist_plugin' } ] }
        is_ex = False
        try:
            self.wps.execute( 'get_option_descriptions', plugin_arg )
            
        except RuntimeError as e:
            is_ex=True
        self.assertEqual( is_ex, True )

        # route that passes near a public transport line (Nantes data)
        ox = 356241.225231
        oy = 6688158.053382
        dx = 355763.149926
        dy = 6688721.876838

        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )

        tempus.request( plugin_name = 'sample_road_plugin',
                        plugin_options = { 'trace_vertex' : True },
                        origin = Point( ox, oy ),
                        steps = [ RequestStep(destination = Point(dx, dy)) ] )
        self.assertEqual( len(tempus.results[0].steps), 5 )

        # run without options
        is_ex = False
        try:
            tempus.request( plugin_name = 'sample_road_plugin',
                            origin = Point( ox, oy ),
                            steps = [ RequestStep(destination = Point(dx, dy)) ] )
        except RuntimeError as e:
            is_ex=True
        self.assertEqual( is_ex, False )


    def test_constants( self ):
        outputs = self.wps.execute( 'constant_list', {} )

    def test_multi_plugin( self ):

        # two road nodes that are linked to a public transport stop
        ox = 355349.238904
        oy = 6689349.662689
        dx = 356132.718582
        dy = 6687761.706782

        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )
        tempus.request( plugin_name = 'sample_multi_plugin',
                        origin = Point( ox, oy ),
                        steps = [ RequestStep(destination = Point(dx, dy)) ] )
        self.assertEqual( len(tempus.results[0].steps), 8 )

    def test_pt_plugin( self ):

        tempus = TempusRequest( 'http://' + WPS_HOST + WPS_PATH )
        tempus.request( plugin_name = 'sample_pt_plugin',
                        plugin_options = { 'origin_pt_stop' : 2475,
                                           'destination_pt_stop' : 1318 },
                        origin = Point( vertex = 0 ),
                        steps = [ RequestStep(destination = Point( vertex = 0 )) ] )
        self.assertEqual( len(tempus.results[0].steps), 1 )

if __name__ == '__main__':
    unittest.main()


