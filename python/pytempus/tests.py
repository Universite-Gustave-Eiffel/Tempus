import pytempus as tempus
import unittest
import os

g_db_options = os.getenv('TEMPUS_DB_OPTIONS', '')
g_db_name = os.getenv('TEMPUS_DB_NAME', 'tempus_test_db')

class CreatePlugin(unittest.TestCase):
	def test(self):
		class PyPlugin(tempus.Plugin):
			def __init__(self, progress, options):
				tempus.Plugin.__init__(self, PyPlugin.name())
				self.progress = progress
				self.options = options

			@staticmethod
			def create_plugin(progression, options):
			    return PyPlugin(progression, options)

			@staticmethod
			def options_description():
				o1 = tempus.Plugin.OptionDescription()
				o1.description = 'option1 is the first option'
				o1.default_value = 42
				o1.visible = True

				o2 = tempus.Plugin.OptionDescription()
				o2.description = 'option2 is the second option'
				o2.default_value = 'text'
				o2.visible = False

				return { 'option1': o1, 'option2': o2 }

			@staticmethod
			def capabilities():
				capa = tempus.Plugin.Capabilities()
				capa.depart_after = True
				return capa

			@staticmethod
			def name():
			    return 'py_test_plugin'

		tempus.PluginFactory.instance().register_plugin_fn(PyPlugin.create_plugin, PyPlugin.options_description, PyPlugin.capabilities, PyPlugin.name)

		options = { 'i': 12, 's': 'text', 'f': 1.2 }

		plugin = tempus.PluginFactory.instance().create_plugin(
			'py_test_plugin',
			tempus.TextProgression(50),
			options)

		self.assertEqual(plugin.name(), PyPlugin.name())
		self.assertEqual(True, tempus.PluginFactory.instance().plugin_capabilities(PyPlugin.name()).depart_after)

		self.assertEqual(True, tempus.PluginFactory.instance().option_descriptions(PyPlugin.name())['option1'].visible)
		self.assertEqual(42, tempus.PluginFactory.instance().option_descriptions(PyPlugin.name())['option1'].default_value)
		self.assertEqual('text', tempus.PluginFactory.instance().option_descriptions(PyPlugin.name())['option2'].default_value)
		self.assertEqual('option2 is the second option', tempus.PluginFactory.instance().option_descriptions(PyPlugin.name())['option2'].description)
		self.assertEqual(False, tempus.PluginFactory.instance().option_descriptions(PyPlugin.name())['option2'].visible)
		self.assertEqual(1.2, plugin.options['f'])


class TestMultimodal(unittest.TestCase):
	def test(self):
		progression = tempus.TextProgression(50)
		options = {'db/options': '{} dbname = {}'.format(g_db_options, g_db_name)}
		graph = tempus.load_routing_data("multimodal_graph", progression, options)

		nv = 0
		n_road_vertices = 0
		n_pt_vertices = 0
		n_pois = 0

		for v in tempus.Multimodal.vertices(graph):
			nv += 1
			if v.type == tempus.Multimodal.Vertex.VertexType.Road:
				n_road_vertices += 1
			elif v.type == tempus.Multimodal.Vertex.VertexType.PublicTransport:
				n_pt_vertices += 1
			else:
				n_pois += 1

		print('nv = {}'.format(nv))
		print('n_road_vertices = {} num_vertices(road) = {}'.format(n_road_vertices, tempus.Road.Graph.num_vertices(graph.road())))
		print('n_pt_vertices = {} num_vertices(pt) = {}'.format(n_pt_vertices, -1)) #tempus.Road.num_vertices(graph.road))
		print('n_pois = {} pois.size() = {}'.format(n_pois, len(graph.pois())))
		print('num_vertices = {}'.format(tempus.Multimodal.num_vertices(graph)))
		self.assertEqual(n_road_vertices, tempus.Road.Graph.num_vertices(graph.road()))
		self.assertEqual(n_pois, len(graph.pois()))
		self.assertEqual(nv, tempus.Multimodal.num_vertices(graph))


		total_out = 0
		total_in = 0
		for v in tempus.Multimodal.vertices(graph):
			n_out = len([1 for e in tempus.Multimodal.out_edges(v, graph)])
			self.assertEqual(n_out, tempus.Multimodal.out_degree(v, graph))
			total_out += n_out

			n_in = len([1 for e in tempus.Multimodal.in_edges(v, graph)])
			self.assertEqual(n_in, tempus.Multimodal.in_degree(v, graph))
			total_in += n_in

			self.assertEqual(n_out + n_in, tempus.Multimodal.degree(v, graph))

		self.assertEqual(total_out, total_in)


if __name__ == '__main__':
	unittest.main()
