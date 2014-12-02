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
# for PostgreSQL connection
import psycopg2
import argparse
import random
import time
import os
import sys

# add ../../python to the include dir
script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../python' )
sys.path.insert(0, wps_path)
from tempus_request import *
from history_file import *

dbstring="dbname=tempus_test_db"
wpsurl="http://127.0.0.1/wps"
npts=100

# test command line arguments
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tempus batch')
    parser.add_argument('-d', '--dbstring', required=False,
            help='The database connection string')
    parser.add_argument('-w', '--wpsurl', required=False,
            help='The WPS server URL')
    parser.add_argument('-n', '--npts', required=False,
            help='Number of point pairs to test')

    args = parser.parse_args()

    if args.dbstring:
        dbstring = args.dbstring
    if args.wpsurl:
        wpsurl = args.wpsurl
    if args.npts:
        npts = int(args.npts)

    tempus = TempusRequest( wpsurl )

    # connection to db
    conn = psycopg2.connect( dbstring )
    cur = conn.cursor()

    # get every road node
    cur.execute( "SELECT id, st_x(geom) as x, st_y(geom) as y FROM tempus.road_node" )
    points = []
    for pt in cur:
        points.append( Point(vertex=int(pt[0])) )

    random.seed()

    server_state_xml = tempus.server_state()
    history = ZipHistoryFile( "history.save" )

    times = []
    distances = []
    total_time = 0
    total_rtime = 0
    for i in range(0,npts):

        # pick two points at random and ask for an itinerary
        n = len(points)
        dep = points[ random.randrange(n) ]
        arr = points[ random.randrange(n) ]

        # distance computation
        dist = (dep.x-arr.x)**2 + (dep.y-arr.y)**2

        start_time = time.time()

        # the actual request
        try:
            req_xml = tempus.request( plugin_name = "sample_road_plugin",
                                      plugin_options = { 'prepare_result' : True },
                                      origin = dep,
                                      steps = [RequestStep(destination = arr)] )
        except RuntimeError as e:
            if e[1].find('No path found') > -1:
                # ignore error when no path has been found
                continue
            else:
                print e[1]
                exit(1)

        # time spend by the algorithm
        t = float(tempus.metrics['time_s'])
        total_time = total_time + t

        # time spend by all the WPS communication
        r = time.time() - start_time
        total_rtime += r

        # total cost
        rDistance = 0.0
        if len(tempus.results) > 0:
            rDistance =  tempus.results[0].costs[Cost.Distance]

        print "i=%d, t=%f, r=%f, d=%f" % (i, t, r, rDistance)

        times.append(t)
        distances.append(rDistance)

        # save request
        history.addRecord( '<record>' + req_xml + server_state_xml + '</record>' )

    print "Total time spend by plugin: %.2fs" % total_time
    print "Total time (with wps communication): %.2fs" % total_rtime


