#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
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
# for PostgreSQL connection
import argparse
import os
import sys

# add ../../python to the include dir
script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../python' )
sys.path.insert(0, wps_path)
from tempus_request import *
from history_file import *
from wps_client import *

from xml.etree import ElementTree as ET

dbstring="dbname=tempus_test_db"
wpshost = "127.0.0.1"
wpsurl="/wps"
savefile="history.save"

# test command line arguments
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='This script allows to read a raw Tempus XML request and re-execute it on a Tempus server. Results are saved in a history file loadable by the QGIS plugin.')
    parser.add_argument('-d', '--dbstring', required=False,
            help='The database connection string')
    parser.add_argument('-w', '--wpsurl', required=False,
            help='The WPS server URL')
    parser.add_argument('-H', '--wpshost', required=False,
            help='The WPS server host')
    parser.add_argument('-r', '--request', required=True,
            help='The XML request file')
    parser.add_argument('-s', '--savefile', required=False,
            help='The file where to save the result')

    args = parser.parse_args()

    if args.dbstring:
        dbstring = args.dbstring
    if args.wpsurl:
        wpsurl = args.wpsurl
    if args.wpshost:
        wpshost = args.wpshost
    if args.savefile:
        savefile = args.savefile

    tempus = TempusRequest( "http://" + wpshost + wpsurl )
    server_state_xml = tempus.server_state()
    history = ZipHistoryFile("history.save")

    conn = HttpCgiConnection( wpshost, wpsurl )
    with open( args.request, 'r' ) as f:
        req = f.read()
        xml = ET.XML(req)

        request_xml = xml[1][0][1][0][0]
        options_xml = xml[1][1][1][0][0]
        plugin_xml = xml[1][2][1][0][0]
        
        status, content = conn.request("POST", req)

        o_xml = ET.XML(content)

        metrics_xml = o_xml[2][0][2][0][0]
        results_xml = o_xml[2][1][2][0][0]
        req_xml = "<select>" + ET.tostring(plugin_xml) + ET.tostring(request_xml) + ET.tostring(options_xml) + ET.tostring(results_xml) + ET.tostring(metrics_xml) + "</select>"
        history.addRecord('<record>' + req_xml + server_state_xml + '</record>')



