import datetime
import os
import sys
import re

# add ../../wps/client to the include dir
script_path = os.path.abspath(os.path.dirname(sys.argv[0]))
wps_path = os.path.abspath( script_path + '/../wps/client' )
sys.path.insert(0, wps_path)
from wps_client import *

class Point:
    def __init__( self, x=0.0, y=0.0, vertex=-1 ):
        self.x = x
        self.y = y
        self.vertex = vertex
    def to_pson( self, name = 'point' ):
        if self.vertex == -1:
            return [ name, { 'x' : str(self.x), 'y' : str(self.y) } ]
        return [ name, { 'vertex' : str(self.vertex) } ]

class DateTime( datetime.datetime ):
    def __init__(self, *args):
        datetime.datetime.__init__( self, args )

    @classmethod
    def now(cls):
        n = datetime.datetime.now()
        return cls(n.year, n.month, n.day, n.hour, n.minute, n.second, n.microsecond)

    def __str__( self ):
        return "%04d-%02d-%02dT%02d:%02d:%02d.0" % (self.year, self.month, self.day, self.hour, self.minute, self.second)
    
class Constraint:
    def __init__( self, type = 0, date_time = DateTime.now() ):
        self.type = type
        self.date_time = date_time

    def to_pson( self ):
        return [ 'constraint', {'type':str(self.type), 'date_time': str(self.date_time)}]

class RequestStep:
    def __init__( self, destination = Point(), constraint = Constraint(), private_vehicule_at_destination = False ):
        self.destination = destination
        self.constraint = constraint
        self.private_vehicule_at_destination = private_vehicule_at_destination

    def to_pson( self ):
        return ['step',
                { 'private_vehicule_at_destination' : 'true' if self.private_vehicule_at_destination else 'false' },
                self.destination.to_pson('destination'),
                self.constraint.to_pson(),
                ]

class RoadStep:
    def __init__( self, road='', end_movement=0, costs={}, wkb='' ):
        self.road = road
        self.end_movement = end_movement
        self.costs = costs
        self.wkb = wkb

class PublicTransportStep:
    def __init__( self, network = '', departure = '', arrival = '', trip = '', costs = {}, wkb = '' ):
        self.network = network
        self.departure = departure
        self.arrival = arrival
        self.trip = trip
        self.costs = costs
        self.wkb = wkb

class RoadTransportStep:
    def __init__( self, type = 0, road = '', network = '', stop = '', wkb = ''):
        self.type = type
        self.road = road
        self.network = network
        self.stop = stop
        self.wkb = wkb

class Result:
    def __init__( self, steps = [], costs = {} ):
        self.steps = steps
        self.costs = costs

class TempusRequest:

    def __init__( self, wps_url = "http://127.0.0.1/wps" ):
        g = re.search( 'http://([^/]+)(.*)', wps_url )
        host = g.group(1)
        path = g.group(2)
        httpClient = HttpCgiConnection( host, path )
        self.wps = WPSClient( httpClient )
        # test wps connection
        [r, msg] = self.wps.get_capabilities()
        if r != 200:
            raise RuntimeError("WPS connection problem: " + msg)

    def request( self, plugin_name = 'sample_road_plugin',
                 plugin_options = {},
                 origin = Point(),
                 departure_constraint = Constraint(),
                 steps = [RequestStep()] ):

        def parse_metrics( metrics ):
            m = {}
            for metric in metrics:
                m[metric.attrib['name']] = metric.attrib['value']
            return m

        def parse_results( results ):
            r = []
            for result in results:
                steps = []
                gcosts = {}
                for child in result:
                    if child.tag == 'road_step':
                        costs = {}
                        road = ''
                        movement = 0
                        wkb = ''
                        for p in child:
                            if p.tag == 'road':
                                road = p.text
                            elif p.tag == 'end_movement':
                                movement = int(p.text)
                            elif p.tag == 'cost':
                                costs[int(p.attrib['type'])] = float(p.attrib['value'])
                            elif p.tag == 'wkb':
                                wkb = wkb
                        steps.append( RoadStep( road = road,
                                                end_movement = movement,
                                                costs = costs,
                                                wkb = wkb) )
                    if child.tag == 'public_transport_step':
                        costs = {}
                        wkb = ''
                        departure = ''
                        arrival = ''
                        trip = ''
                        for p in child:
                            if p.tag == 'network':
                                network = p.text
                            elif p.tag == 'departure_stop':
                                departure = p.text
                            elif p.tag == 'arrival_stop':
                                arrival = p.text
                            elif p.tag == 'trip':
                                trip = p.text
                            elif p.tag == 'cost':
                                costs[int(p.attrib['type'])] = float(p.attrib['value'])
                            elif p.tag == 'wkb':
                                wkb = wkb
                        steps.append( PublicTransportStep( network = network,
                                                           departure = departure,
                                                           arrival = arrival,
                                                           trip = trip,
                                                           costs = costs,
                                                           wkb = wkb) )
                    if child.tag == 'road_transport_step':
                        costs = {}
                        wkb = ''
                        road = ''
                        network = ''
                        stop = ''
                        type = 0
                        for p in child:
                            if p.tag == 'network':
                                network = p.text
                            elif p.tag == 'type':
                                type = int(p.text)
                            elif p.tag == 'road':
                                road = p.text
                            elif p.tag == 'stop':
                                stop = p.text
                            elif p.tag == 'cost':
                                costs[int(p.attrib['type'])] = float(p.attrib['value'])
                            elif p.tag == 'wkb':
                                wkb = wkb
                        steps.append( RoadTransportStep( network = network,
                                                         type = type,
                                                         road = road,
                                                         stop = stop,
                                                         costs = costs,
                                                         wkb = wkb) )
                    elif child.tag == 'cost':
                        gcosts[int(child.attrib['type'])] = float(child.attrib['value'])
                r.append( Result( steps = steps, costs = gcosts ) )
            return r

        args = {}
        args['plugin'] = ['plugin', {'name' : plugin_name } ]
        args['request'] = ['request',
                           {'allowed_transport_types' : 11 },
                           origin.to_pson( 'origin' ),
                           ['departure_constraint', { 'type': departure_constraint.type, 'date_time': str(departure_constraint.date_time) } ],
                           ['optimizing_criterion', 1 ], 
                           ]
        for step in steps:
            args['request'].append( step.to_pson() )

        # options
        opt_r = []
        for k,v in plugin_options.iteritems():
            opt_r.append( [ 'option', {'name':k, 'value':str(v)}] )
        opt_r.insert(0, 'options')
        args['options'] = opt_r

        outputs = self.wps.execute( 'select', args )

        self.metrics = parse_metrics( outputs['metrics'] )
        self.results = parse_results( outputs['results'] )

