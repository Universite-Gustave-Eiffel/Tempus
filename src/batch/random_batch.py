#!/usr/bin/env python
import sys
import os
import re

# add ../../wps/client to the include dir
script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../wps/client' )
sys.path.insert(0, wps_path)
from wps_client import *

# for PostgreSQL connection
import psycopg2
import argparse
import random

dbstring="dbname=tempus_test_db"
wpsurl="http://127.0.0.1/wps"

def parse_metrics( metrics ):
    m = {}
    for metric in metrics:
        m[metric.attrib['name']] = metric.attrib['value']
    return m

# test command line arguments
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tempus batch')
    parser.add_argument('-d', '--dbstring', required=False,
            help='The database connection string')
    parser.add_argument('-w', '--wpsurl', required=False,
            help='The WPS server URL')

    args = parser.parse_args()

    if args.dbstring:
        dbstring = args.dbstring
    if args.wpsurl:
        wpsurl = args.wpsurl

    # connection to db
    conn = psycopg2.connect( dbstring )
    cur = conn.cursor()

    # connection to WPS
    g = re.search( 'http://([^/]+)(.*)', wpsurl )
    host = g.group(1)
    path = g.group(2)
    httpClient = HttpCgiConnection( host, path )
    wps = WPSClient( httpClient )
    # test wps connection
    [r, msg] = wps.get_capabilities()
    if r != 200:
        print "WPS connection problem: ", msg
        exit(0)

    # get every road node
    cur.execute( "SELECT st_x(geom) as x, st_y(geom) as y FROM tempus.road_node" )
    points = []
    for pt in cur:
        points.append( (float(pt[0]), float(pt[1])) )

    # pick two points at random and ask for an itinerary
    random.seed()

    total_time = 0
    for i in range(0,100):
        n = len(points)
        dep = points[ random.randrange(n) ]
        arr = points[ random.randrange(n) ]

        # build a request
        args = {}
        args['plugin'] = ['plugin', {'name' : 'sample_road_plugin' } ]
        args['request'] = ['request', 
                           ['origin', ['x', str(dep[0])], ['y', str(dep[1])] ],
                           ['departure_constraint', { 'type': 0, 'date_time': '2012-03-14T11:05:34' } ],
                           ['optimizing_criterion', 1 ], 
                           ['allowed_transport_types', 11 ],
                           ['step',
                            [ 'destination', ['x', str(arr[0])], ['y', str(arr[1])] ],
                            [ 'constraint', { 'type' : 0, 'date_time':'2012-04-23T00:00:00' } ],
                            [ 'private_vehicule_at_destination', 'true' ]
                            ]
                           ]
        # requested argument
        args['options'] = ['options']

        outputs = wps.execute( 'select', args )

        metrics = parse_metrics( outputs['metrics'] )
        if metrics.has_key('time_s'):
            total_time = total_time + float(metrics['time_s'])
            print total_time
        else:
            print 'no path'

