#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *   Copyright (C) 2015 Mappy <dt.lbs.route@mappy.com>
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

Script used to test the CH plugin. Results are compared to a simple dijkstra request using sample_road_plugin
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
wps_path = os.path.abspath(script_path + '/../../python')
sys.path.insert(0, wps_path)

from tempus_request import *
from history_file import *

dbstring = "dbname=tempus_test_db"
wpsurl = "http://127.0.0.1/wps"
npts = 100

# test command line arguments
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tempus batch')
    parser.add_argument(
        '-d', '--dbstring', required=False,
        help='The database connection string')
    parser.add_argument(
        '-w', '--wpsurl', required=False,
        help='The WPS server URL')
    parser.add_argument(
        '-n', '--npts', required=False,
        help='Number of point pairs to test')

    args = parser.parse_args()

    if args.dbstring:
        dbstring = args.dbstring
    if args.wpsurl:
        wpsurl = args.wpsurl
    if args.npts:
        npts = int(args.npts)

    tempus = TempusRequest(wpsurl)

    # connection to db
    conn = psycopg2.connect(dbstring)
    cur = conn.cursor()

    # get every road node
    # from road sections only available to pedestrians
    cur.execute("""select rn.id, st_x(rn.geom) as x, st_y(rn.geom) as y from tempus.road_node as rn
left join tempus.road_section as rs1 on rs1.node_from = rn.id
left join tempus.road_section as rs2 on rs2.node_to = rn.id 
where
(rs1.traffic_rules_ft & 1 > 0
or rs2.traffic_rules_ft & 1 > 0)
and
(rs1.node_from is not null or rs2.node_to is not null)""")
    points = []
    for pt in cur:
        points.append(Point(vertex=int(pt[0])))

    random.seed()

    server_state_xml = tempus.server_state()
    history = ZipHistoryFile("history.save")

    times = []
    distances = []
    total_time = 0
    total_rtime = 0
    distance = {}
    elapsed = {}
    path = {}
    t_dijkstra = 0
    t_ch = 0
    print("Randomly choosing %d pairs of points and comparing Dijkstra and CH results" % npts)
    for i in range(0, npts):

        # pick two points at random and ask for an itinerary
        n = len(points)
        dep = points[random.randrange(n)]
        arr = points[random.randrange(n)]
        print "From", dep.vertex, " to ", arr.vertex

        for plugin in ("sample_road_plugin", "ch_plugin"):
            # reference request
            try:
                req_xml = tempus.request(
                    plugin_name=plugin,
                    plugin_options={'prepare_result': True},
                    origin=dep,
                    steps=[RequestStep(destination=arr)]
                )
            except RuntimeError as e:
                if e[1].find('No path found') > -1:
                # ignore error when no path has been found
                    distance[plugin] = 0
                    path[plugin] = []
                    print "no path found"
                    continue
                else:
                    print e[1]
                    exit(1)

            # time spend by the algorithm
            t = float(tempus.metrics['time_s'])
            elapsed[plugin] = t

            # total cost
            rDistance = 0.0
            if len(tempus.results) > 0:
                rDistance = tempus.results[0].costs[Cost.Distance]

            path[plugin]=list(tempus.results[0].steps)

            #print plugin, rDistance, t
            distance[plugin] = rDistance

            history.addRecord('<record>' + req_xml + server_state_xml + '</record>')

        print 'Dijkstra:', distance['sample_road_plugin'], 'CH:', distance['ch_plugin']
        if distance['sample_road_plugin'] == 0.0:
            continue

        t_dijkstra += elapsed["sample_road_plugin"]
        t_ch += elapsed["ch_plugin"]
        diff = abs(distance['sample_road_plugin'] - distance['ch_plugin']) / distance['sample_road_plugin']
        if diff > 0.01: # more than 1% of relative error
            print "Difference"
            pi = path['sample_road_plugin']
            pj = path['ch_plugin']
            for i in range(min(len(pi), len(pj))):
                tdi = pi[i].costs[Cost.Distance]
                tdj = pj[i].costs[Cost.Distance]
                print pi[i].road, pj[i].road
            break

    print
    print("Dijkstra average time: %.3fs" % (t_dijkstra / npts))
    print("CH average time: %.3fs" % (t_ch / npts))
    print("CH is %.1f faster than Dijkstra on average" % (t_dijkstra / t_ch))



