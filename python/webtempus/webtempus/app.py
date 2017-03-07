# -*- coding: utf-8 -*-
import importlib
from flask import Flask, request
import pytempus as tempus

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

@app.route('/request')
def make_request():

  print(request.args['start'])
  print(request.args['dest'])
  return 'Itinerary: '

