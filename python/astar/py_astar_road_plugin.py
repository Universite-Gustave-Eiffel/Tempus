import tempus

class PyAStarRequest(tempus.PluginRequest):
	def __init__(self, plugin, options):
		tempus.PluginRequest.__init__(self, plugin, options)

	def process(self, request):
		r = tempus.ResultElement()
		return [r]


class AStar(tempus.Plugin):
	def __init__(self, progression, options):
		tempus.Plugin.__init__(self, 'py_astar_road_plugin')

	def request(self, opts):
		return PyAStarRequest(self, opts)


def create_plugin(progression, options):
	plugin = AStar(progression, options)
	return plugin

def options_description():
	print ('options_description')
	return {}

def capabilities():
	print ('capabilities')
	return {}

def name():
	return 'py_astar_road_plugin'
