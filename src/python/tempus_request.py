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


import datetime
import os
import sys
import re

from wps_client import *

class Cost:
    Distance = 1
    Duration = 2
    Price = 3
    Carbon = 4
    Calories = 5
    NumberOfChanges = 6
    Variability = 7

CostName = {
    1 : 'Distance',
    2 : 'Duration',
    3 : 'Price',
    4 : 'Carbon',
    5 : 'Calories',
    6 : 'NumberOfChanges',
    7 : 'Variability'
}

CostUnit = {
    1 : 'm',
    2 : 'min',
    3 : '€',
    4 : 'kg',
    5 : 'kcal',
    6 : '',
    7 : ''
}

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

class EndMovement:
    GoAhead = 0
    TurnLeft = 1
    TurnRight = 2
    UTurn = 3
    RoundAboutEnter = 4
    FirstExit = 5
    SecondExit = 6
    ThirdExit = 7
    FourthExit = 8
    FifthExit = 9
    SixthExit = 10
    YouAreArrived = 999

class RoadStep:
    def __init__( self, road='', end_movement=0, costs={}, mode=0, wkb='' ):
        self.road = road
        self.end_movement = end_movement
        self.costs = costs
        self.mode = mode
        self.wkb = wkb

class PublicTransportStep:
    def __init__( self, network = '', departure = '', arrival = '', route = '', trip_id = 0, costs = {}, mode = 0, departure_time = 0.0, arrival_time = 0.0, wait_time = 0.0, wkb = '' ):
        self.network = network
        self.departure = departure
        self.arrival = arrival
        self.route = route
        self.trip_id = trip_id
        self.departure_time = departure_time
        self.arrival_time = arrival_time
        self.wait_time = wait_time
        self.costs = costs
        self.mode = mode
        self.wkb = wkb

class ConnectionType:
    Unknown = 0
    Road2Road = 1
    Road2Transport = 2
    Transport2Road = 3
    Transport2Transport = 4
    Road2Poi = 5
    Poi2Road = 6

class RoadTransportStep:
    def __init__( self, type = 0, road = '', network = '', stop = '', costs = {}, mode = 0, wkb = ''):
        self.type = type
        self.road = road
        self.network = network
        self.stop = stop
        self.costs = costs
        self.mode = mode
        self.wkb = wkb

class TransferStep:
    def __init__( self, type = 0, road = '', poi = '', mode = 0, final_mode = 0, costs = {}, wkb= '' ):
        self.type = type
        self.road = road
        self.poi = poi
        self.final_mode = final_mode
        self.costs = costs
        self.mode = mode
        self.wkb = wkb

class Result:
    # cost : id(int) => value(float)
    def __init__( self, steps = [], costs = {}, starting_date_time = '', trace = None ):
        self.steps = steps
        self.costs = costs
        dt = datetime.datetime.strptime(starting_date_time, '%Y-%m-%dT%H:%M:%S')
        self.starting_date_time = DateTime( dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second )
        self.trace = trace

class OptionType:
    Bool = 0,
    Int = 1,
    Float = 2,
    String = 3

class OptionValue:
    def __init__( self, value = 0, type = OptionType.Int ):
        self.value = value
        self.type = type

class Variant:
    def __init__( self, value ):
        self.value = value

class PluginOption:
    def __init__( self, name = '', description = '', default_value = OptionValue() ):
        self.name = name
        self.description = description
        self.default_value = default_value
    def type( self ):
        return self.default_value.type

# options : option_name => PluginOption
class Plugin:
    def __init__( self, name = '', options = {}, supported_criteria = [], intermediate_steps = False, depart_after = False, arrive_before = False ):
        self.name = name
        self.options = options
        self.supported_criteria = supported_criteria
        self.intermediate_steps = intermediate_steps
        self.depart_after = depart_after
        self.arrive_before = arrive_before

class TransportMode:
    def __init__( self, id = 0, name = '', is_public_transport = False, need_parking = False, is_shared = False, must_be_returned = False, traffic_rules = 0, speed_rule = 0, toll_rules = 0, engine_type = 0 ):
        self.id = id
        self.name = name
        self.is_public_transport = is_public_transport
        self.need_parking = need_parking
        self.is_shared = is_shared
        self.must_be_returned = must_be_returned
        self.traffic_rules = traffic_rules
        self.speed_rule = speed_rule
        self.toll_rules = toll_rules
        self.engine_type = engine_type

class TransportNetwork:
    def __init__( self, id = 0, name = '' ):
        self.id = id
        self.name = name

class RoadVertex:
    def __init__( self, id ):
        self.id = id

class PoiVertex:
    def __init__( self, id ):
        self.id = id

class PtVertex:
    def __init__( self, id, network = 1):
        self.id = id
        self.network = network

class ValuedEdge:
    def __init__( self, origin = None, destination = None, wkb = '', variants = {} ):
        self.origin = origin
        self.destination = destination
        self.wkb = wkb
        self.variants = variants

def parse_vertex( v ):
    if v.tag == 'road':
        return RoadVertex( int(v.attrib['id']) )
    elif v.tag == 'poi':
        return PoiVertex( int(v.attrib['id']) )
    elif v.tag == 'pt':
        if 'network' in v.attrib:
            return PtVertex( int(v.attrib['id']), int(v.attrib['network']) )
        else:
            return PtVertex( int(v.attrib['id']) )

def parse_variants( variants ):
    v = {}
    for x in variants:
        if x.tag == 'b':
            v[x.attrib['k']] = x.attrib['v'] == 'true'
        elif x.tag == 'i':
            v[x.attrib['k']] = int(x.attrib['v'])
        elif x.tag == 'f':
            v[x.attrib['k']] = float(x.attrib['v'])
        elif x.tag == 's':
            v[x.attrib['k']] = x.attrib['s']
    return v

def parse_option_value( opt ):
    optval = None
    type = 0
    if opt.tag == 'bool_value':
        optval = opt.attrib['value'] == "true"
        type = OptionType.Bool
    elif opt.tag == 'int_value':
        optval = int(opt.attrib['value'])
        type = OptionType.Int
    elif opt.tag == 'float_value':
        optval = float(opt.attrib['value'])
        type = OptionType.Float
    elif opt.tag == 'string_value':
        optval = opt.attrib['value']
        type = OptionType.String
    return OptionValue( value = optval, type=type)

def parse_plugins( output ):
    plugins = []
    for plugin in output:
        name = plugin.attrib['name']
        options = {}
        supported_criteria = []
        intermediate_steps = False
        depart_after = False
        arrive_before = False
        for option in plugin:
            if option.tag == 'option':
                optval = parse_option_value( option[0][0] )
                options[option.attrib['name']] = PluginOption( name = option.attrib['name'],
                                                               description = option.attrib['description'],
                                                               default_value = optval )
            if option.tag == 'supported_criterion':
                supported_criteria.append( int(option.text) )
            if option.tag == 'intermediate_steps':
                intermediate_steps = option.text == "true"
            if option.tag == 'depart_after':
                depart_after = option.text == "true"
            if option.tag == 'arrive_before':
                arrive_before = option.text == "true"
        plugins.append( Plugin( name = name,
                                options = options,
                                supported_criteria = supported_criteria,
                                intermediate_steps = intermediate_steps,
                                depart_after = depart_after,
                                arrive_before = arrive_before
                            ) )
    return plugins

def parse_transport_modes( output ):
    return [ TransportMode( id = int(x.attrib['id']),
                            is_public_transport = x.attrib['is_public_transport'] == "1",
                            name = x.attrib['name'],
                            need_parking = x.attrib['need_parking'] == "1",
                            is_shared = x.attrib['is_shared'] == "1",
                            must_be_returned = x.attrib['must_be_returned'] == "1",
                            traffic_rules = int(x.attrib['traffic_rules']),
                            speed_rule = int(x.attrib['speed_rule']),
                            toll_rules = int(x.attrib['toll_rules']),
                            engine_type = int(x.attrib['engine_type']) )
             for x in output ]
def parse_transport_networks( output ):
    return [ TransportNetwork( id = int(x.attrib['id']),
                               name = x.attrib['name'] )
             for x in output ]

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
        trace = []
        starting_dt = ''
        for child in result:
            if child.tag == 'road_step':
                costs = {}
                road = child.attrib['road']
                movement = int(child.attrib['end_movement'])
                transport_mode = int(child.attrib['transport_mode'])
                wkb = child.attrib['wkb']
                for p in child:
                    if p.tag == 'cost':
                        costs[int(p.attrib['type'])] = float(p.attrib['value'])
                steps.append( RoadStep( road = road,
                                        end_movement = movement,
                                        costs = costs,
                                        mode = transport_mode,
                                        wkb = wkb) )
            if child.tag == 'public_transport_step':
                costs = {}
                wkb = child.attrib['wkb']
                departure = child.attrib['departure_stop']
                arrival = child.attrib['arrival_stop']
                route = child.attrib['route']
                trip_id = int(child.attrib['trip_id'])
                network = child.attrib['network']
                transport_mode = int(child.attrib['transport_mode'])
                departure_time = float(child.attrib['departure_time'])
                arrival_time = float(child.attrib['arrival_time'])
                wait_time = float(child.attrib['wait_time'])
                for p in child:
                    if p.tag == 'cost':
                        costs[int(p.attrib['type'])] = float(p.attrib['value'])
                steps.append( PublicTransportStep( network = network,
                                                   departure = departure,
                                                   arrival = arrival,
                                                   route = route,
                                                   trip_id = trip_id,
                                                   departure_time = departure_time,
                                                   arrival_time = arrival_time,
                                                   wait_time = wait_time,
                                                   costs = costs,
                                                   mode = transport_mode,
                                                   wkb = wkb) )
            if child.tag == 'road_transport_step':
                costs = {}
                wkb = child.attrib['wkb']
                road = child.attrib['road']
                network = child.attrib['network']
                stop = child.attrib['stop']
                type = int(child.attrib['type'])
                transport_mode = int(child.attrib['transport_mode'])
                for p in child:
                    if p.tag == 'cost':
                        costs[int(p.attrib['type'])] = float(p.attrib['value'])
                steps.append( RoadTransportStep( network = network,
                                                 type = type,
                                                 road = road,
                                                 stop = stop,
                                                 costs = costs,
                                                 mode = transport_mode,
                                                 wkb = wkb) )
            if child.tag == 'transfer_step':
                costs = {}
                wkb = child.attrib['wkb']
                road = child.attrib['road']
                poi = child.attrib['poi']
                type = int(child.attrib['type'])
                transport_mode = int(child.attrib['transport_mode'])
                final_mode = int(child.attrib['final_mode'])
                for p in child:
                    if p.tag == 'cost':
                        costs[int(p.attrib['type'])] = float(p.attrib['value'])
                        steps.append( TransferStep( type = type,
                                                    road = road,
                                                    poi = poi,
                                                    costs = costs,
                                                    mode = transport_mode,
                                                    final_mode = final_mode,
                                                    wkb = wkb) )
            elif child.tag == 'cost':
                gcosts[int(child.attrib['type'])] = float(child.attrib['value'])
            elif child.tag == 'starting_date_time':
                starting_dt = child.text
            elif child.tag == 'trace':
                # for each edge
                edges = child
                for e in child:
                    wkb = e.attrib['wkb']
                    o = parse_vertex(e[0])
                    d = parse_vertex(e[1])
                    variants = parse_variants(e[2:])
                    vedge = ValuedEdge( origin = o, destination = d, wkb = wkb, variants = variants )
                    trace.append( vedge )
        r.append( Result( steps = steps, costs = gcosts, starting_date_time = starting_dt, trace = trace ) )
    return r

def parse_plugin_options( xml ):
    "Returns a dict of options name => OptionValue"
    options = {}
    for option in xml:
        k = option.attrib['name']
        v = parse_option_value( option[0] )
        options[k] = v
    return options
        

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
            raise RuntimeError(msg)
        self.save = {}

    def plugin_list( self ):
        outputs = self.wps.execute( 'plugin_list', {} )
        self.save['plugins'] = outputs['plugins']
        return parse_plugins( outputs['plugins'] )

    def constant_list( self ):
        outputs = self.wps.execute( 'constant_list', {} )
        for k,v in outputs.iteritems():
            self.save[k] = v
        return ( parse_transport_modes(outputs['transport_modes']),
                 parse_transport_networks(outputs['transport_networks']) )

    def request( self, plugin_name = 'sample_road_plugin',
                 plugin_options = {},
                 origin = Point(),
                 departure_constraint = Constraint(),
                 steps = [RequestStep()],
                 allowed_transport_modes = [1], # at least pedestrian
                 criteria = [Cost.Distance, Cost.Duration], # list of optimizing criteria
                 parking_location = None,
                 networks = [] # public transport network id
                 ):

        args = {}
        args['plugin'] = ['plugin', {'name' : plugin_name } ]
        args['request'] = ['request',
                           origin.to_pson( 'origin' ),
                           ['departure_constraint', { 'type': departure_constraint.type, 'date_time': str(departure_constraint.date_time) } ]
                           ]

        # parking location
        if parking_location:
            args['request'].append( parking_location.to_pson('parking_location') )

        for criterion in criteria:
            args['request'].append( ['optimizing_criterion', criterion] )

        # networks
        for network in networks:
            args['request'].append( ['allowed_network', network] )

        for step in steps:
            args['request'].append( step.to_pson() )

        # allowed modes
        for mode in allowed_transport_modes:
            args['request'].append( ['allowed_mode', mode] )

        # options
        opt_r = []
        for k,v in plugin_options.iteritems():
            tag_name = ''
            value = str(v)
            if isinstance(v, bool):
                tag_name = "bool_value"
                value = "true" if v else "false"
            elif isinstance(v, int):
                tag_name = "int_value"
            elif isinstance(v, float):
                tag_name = "float_value"
            elif isinstance(v, str):
                tag_name = "string_value"
            elif isinstance(v, unicode):
                tag_name = "string_value"
            else:
                raise RuntimeError( "Unknown value type " + value )

            opt_r.append( [ 'option', {'name':k}, [ tag_name, { 'value':value } ] ] )
        opt_r.insert(0, 'options')
        args['options'] = opt_r

        outputs = self.wps.execute( 'select', args )
        for k,v in outputs.iteritems():
            self.save[k] = v

        self.metrics = parse_metrics( outputs['metrics'] )
        self.results = parse_results( outputs['results'] )
        
        r = to_xml(args['plugin']) + to_xml(args['request']) + to_xml(args['options']) + ET.tostring(outputs['results']) + ET.tostring(outputs['metrics'])
        r = "<select>\n" + r + "</select>\n"
        return r


    def server_state( self ):
        """Retrieve current server state and return a XML string"""
        self.plugin_list()
        self.constant_list()

        r = "<server_state>" + ET.tostring(self.save['plugins']) \
            + ET.tostring(self.save['transport_modes']) \
            + ET.tostring(self.save['transport_networks']) \
            + "</server_state>"
        return r

