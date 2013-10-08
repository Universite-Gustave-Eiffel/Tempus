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
    def __init__( self, x=0.0, y=0.0 ):
        self.x = x
        self.y = y

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

class Step:
    def __init__( self, destination = Point(), constraint = Constraint(), private_vehicule_at_destination = False ):
        self.destination = destination
        self.constraint = constraint
        self.private_vehicule_at_destination = private_vehicule_at_destination

    def to_pson( self ):
        return ['step',
                [ 'destination', [ 'x', str(self.destination.x)], ['y', str(self.destination.y)] ],
                self.constraint.to_pson(),
                [ 'private_vehicule_at_destination', 'true' if self.private_vehicule_at_destination else 'false' ]
                ]

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
                 steps = [Step()] ):

        def parse_metrics( metrics ):
            m = {}
            for metric in metrics:
                m[metric.attrib['name']] = metric.attrib['value']
            return m

        args = {}
        args['plugin'] = ['plugin', {'name' : plugin_name } ]
        args['request'] = ['request', 
                           ['origin', ['x', str(origin.x)], ['y', str(origin.y)] ],
                           ['departure_constraint', { 'type': departure_constraint.type, 'date_time': str(departure_constraint.date_time) } ],
                           ['optimizing_criterion', 1 ], 
                           ['allowed_transport_types', 11 ]
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

