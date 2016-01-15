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
import unittest
import argparse
import subprocess

script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
loader_path = os.path.abspath(script_path + '/../src/loader')
data_path = os.path.abspath( script_path + '/../test_data' )
sys.path.insert(0, loader_path)

import tempus
from tempus.config import *

loader = loader_path + "/load_tempus"
dbstring = os.environ.get('DBSTRING') or "dbname=tempus_unit_test"

def get_sql_output( dbstring, sql ):
    cmd = [PSQL, '-d', dbstring, '-t', '-c', sql]
    p = subprocess.Popen(cmd, stdout = subprocess.PIPE )
    return p.stdout.read()

class TestTempusLoader(unittest.TestCase):

    def setUp(self):
        cmd = [PSQL, '-d', dbstring, '-t', '-c', "CREATE EXTENSION IF NOT EXISTS postgis;DROP SCHEMA tempus CASCADE;"]
        print cmd
        subprocess.call( cmd )

    def test_osm_loading( self ):
        r = subprocess.call( ['python', loader, '-t', 'osm', '-s', data_path, '-d', dbstring, '-R'] )
        self.assertEqual(r, 0)

        n_road_nodes = int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_node"))
        self.assertEqual(3172, n_road_nodes)

        n_road_sections = int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_section"))
        self.assertEqual(4027, n_road_sections)

    def test_gtfs_loading( self ):
        # GTFS loading without road
        cmd = ['python', loader, '-t', 'gtfs', '-s', data_path + '/gtfs_min.zip', '-d', dbstring, '-R']
        r = subprocess.call( cmd )
        self.assertEqual(r, 0)

        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_node")), 282)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_section")), 141)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_stop")), 141)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_section")), 69)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_trip")), 3)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_stop_time")), 80)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_route")), 2)

    def test_gtfs_transfers_loading( self ):
        # GTFS loading without road
        r = subprocess.call( ['python', loader, '-t', 'gtfs', '-s', data_path + '/gtfs_min_with_transfers.zip', '-d', dbstring, '-R'] )
        self.assertEqual(r, 0)

        # number of transfers
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_section WHERE road_type=5")), 4)

class TestTempusLoaderProjection(unittest.TestCase):

    def setUp(self):
        cmd = [PSQL, dbstring, '-t', '-c', "CREATE EXTENSION IF NOT EXISTS postgis;DROP SCHEMA tempus CASCADE;"]
        subprocess.call( cmd )

    def test_init(self):
        # create a 4326 db
        r = subprocess.call( [loader, '-d', dbstring, '-R', '-N', '4326'] )
        self.assertEqual(r, 0)

        r = subprocess.call( [loader, '-t', 'osm', '-s', data_path, '-d', dbstring] )
        self.assertEqual(r, 0)

        n_road_nodes = int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_node"))
        self.assertEqual(3176, n_road_nodes)

        n_road_sections = int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.road_section"))
        self.assertEqual(4032, n_road_sections)

        # GTFS loading without road
        r = subprocess.call( [loader, '-t', 'gtfs', '-s', data_path + '/gtfs_min.zip', '-d', dbstring] )
        self.assertEqual(r, 0)

        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_stop")), 141)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_section")), 69)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_trip")), 3)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_stop_time")), 80)
        self.assertEqual(int(get_sql_output(dbstring, "SELECT count(*) FROM tempus.pt_route")), 2)

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Tempus data loader')
    parser.add_argument('-d', '--dbstring', required=False,
            help='The database connection string')
    args = parser.parse_args()

    if args.dbstring is not None:
        dbstring = args.dbstring

    unittest.main()


