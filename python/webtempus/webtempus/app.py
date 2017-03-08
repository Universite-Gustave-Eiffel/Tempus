# -*- coding: utf-8 -*-
import base64
import importlib
import sys
from geomet import wkb
from flask import Flask, request, jsonify
import psycopg2
from psycopg2.extras import RealDictCursor
import pytempus as tempus


# init DB connection
# TODO  config file
config = {}
config['pg_password'] = ''
config['pg_user'] = 'augustin'
config['pg_host'] = 'localhost'
config['pg_port'] = '5432'
config['pg_name'] = 'tempus_test_db'

pg_string = (
    'postgresql://{pg_user}:{pg_password}@{pg_host}:{pg_port}/{pg_name}'
    .format(**config)
)
try:
    connection = psycopg2.connect(pg_string, cursor_factory=RealDictCursor)
    connection.autocommit = True
except psycopg2.Error as error:
    print('Impossible de se connecter au serveur postgres: {}'.format(error))
    sys.exit(1)

# init webapp
app = Flask(__name__)

global plugin


# TODO these are copy-pasted from pytempus. Consider exposing these from there, and using them here?
def load_plugin(name, progress, options):
    try:
        print('Trying to load C++ plugin...', name)
        plugin = tempus.PluginFactory.instance().create_plugin(
            name,
            progress,
            options)
        print('Created C++ plugin [{}]'.format(plugin.name))
        return plugin
    except RuntimeError as e:
        print(e)
        print('Failed... Now trying python...')
        pass # now try a python module

    try:
        py_plugin = importlib.import_module(name)

        tempus.PluginFactory.instance().register_plugin_fn(py_plugin.create_plugin, py_plugin.options_description, py_plugin.capabilities, py_plugin.name)

        plugin = tempus.PluginFactory.instance().create_plugin(
            name,
            progress,
            options)
        print('Created python plugin [{}]'.format(plugin.name))
        return plugin

    except ImportError as e:
        print('Failed as well. Aborting.')
        raise RuntimeError('Failed to load plugin {}'.format(name))



# init python plugin
class Progress(tempus.ProgressionCallback):
    def __call__(self, percent, finished ):
        print ('{}...'.format(percent))

progression = Progress()

options = {}

tempus.init()

# TODO dynamise option
plugin = load_plugin('sample_road_plugin', progression, {'db/options': 'dbname=tempus_test_db'} )


@app.route('/')
def hello_world():
    return 'Hello, World!'


def find_vertex_id(coordinates):
    """
    Parse str from request and find associated vertex id
    """
    query = 'SELECT tempus.road_node_id_from_coordinates({}, {})'.format(coordinates[0], coordinates[1])
    cur = connection.cursor()
    cur.execute(query)
    res = cur.fetchall()
    if not res:
        return None

    return res[0]['road_node_id_from_coordinates']


def parse_coordinates(str):
    xStr, yStr = str.split(';')
    x = float(xStr)
    y = float(yStr)
    return [x, y]


def to_geoJSON(tempusResult):
    # the geoObj contains the object that will be serialized into geojson
    # it's a featureCollection. Each Feature corresponds to a result.
    # Each feature have a GeometryCollection as geometry, that represents a roadmap
    geoObj = { 'type': 'FeatureCollection', 'features': [] }
    for r in tempusResult:
        feature = { 'type': 'Feature', 'properties': {} }
        if r.is_roadmap():
            rm = r.roadmap()
            geometryCollection = { 'type': 'GeometryCollection', 'geometries': [] }
            for s in rm:
                geoJson = wkb.loads(base64.b16decode(s.geometry_wkb.upper()))
                # let's attach some non-standard metadata
                geoJson['tempusStepType'] = s.step_type.name
                geometryCollection['geometries'].append(geoJson)
        else:
            # TODO do something
            geometryCollection = 'null'
        feature['geometry'] = geometryCollection
        geoObj['features'].append(feature)

    return geoObj


@app.route('/request')
def make_request():
    """
    Request an itinerary from tempus. Requests can be either GET or POST, and
    should contains 2 parameters: start and dest, of format 'x;y'
    """

    # Parse request arguments
    try:
        startCoordinates = parse_coordinates(request.args['start'])
        destCoordinates = parse_coordinates(request.args['dest'])
    except ValueError as err:
        print('Error: cannot parse coordinates', err.message)
        return 'Cannot parse coordinates in request', '400'

    # Find the vertex id
    startVertexId = find_vertex_id(startCoordinates)
    destVertexId = find_vertex_id(destCoordinates)

    if not startVertexId:
        return 'Cannot find start point in DB!', '422'
    if not destVertexId:
        return 'Cannot find dest point in DB!', '422'


    req = tempus.Request()
    req.origin = startVertexId
    req.destination = destVertexId

    req.add_allowed_mode(1)

    p = plugin.request(options)

    result = p.process(req)

    return jsonify(to_geoJSON(result))
