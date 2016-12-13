import tempus

class AStar(tempus.Plugin):
	def __init__(self):
		tempus.Plugin.__init__(self, 'py_astar_road_plugin')

	def request(self, opts):
		print ('python request!!!', opts)
		return None


def create_plugin(a, b):
	plugin = AStar()
	print ('CREATE PLUGIN FROM PYTHON')
	return plugin

def options_description():
	print ('options_description')
	return {}

def capabilities():
	print ('capabilities')
	return {}

def name():
	print ('plugin::name...')
	return 'py_astar_road_plugin'
