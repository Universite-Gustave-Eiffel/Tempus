#!/usr/bin/env python
import sys
import os
import re
import unittest

wps_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( wps_path + '/../wps/client' )
sys.path.insert(0, wps_path)
from wps_client import *

WPS_HOST = '127.0.0.1'
WPS_PATH = '/wps'
DB_OPTIONS = ""
DB_NAME = "tempus_test_db"

db_options = "%s dbname=%s" % (DB_OPTIONS, DB_NAME)

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
        [status, msg] = self.wps.describe_process( "pre_process" )
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
            # connect only takes 1 argument
            outputs = self.wps.execute( "connect", { 'arg1' : [True, ''], 'arg2' : [True, 4] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

        # TODO: test bad input argument name
        is_ex = False
        code = 0
        try:
            # connect only takes 1 argument
            outputs = self.wps.execute( "connect", { 'db_optios' : [True, [ 'db_options', db_options] ] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )
        is_ex = False
        code = 0
        try:
            # connect only takes 1 argument
            outputs = self.wps.execute( "connect", { 'db_options' : [True, [ 'db_optis', db_options] ] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )

        # Test non validating input
        is_ex = False
        code = 0
        try:
            # connect only takes 1 argument
            outputs = self.wps.execute( "connect", { 'db_options' : [True, [ 'db_options', {}, [ 'toto', 'ok'] ] ] } )
        except RuntimeError as e:
            [ is_ex, code ] = get_wps_exception( e.args[1] )
        self.assertEqual( is_ex, True )
        self.assertEqual( code, 'InvalidParameterValue' )
        pass

    def test_execution_states(self):
        pass

if __name__ == '__main__':
    unittest.main()


