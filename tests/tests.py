import tempus
import unittest
import os

g_db_options = os.getenv('TEMPUS_DB_OPTIONS', '')
g_db_name = os.getenv('TEMPUS_DB_NAME', 'tempus_test_db')

class TestMultimodal(unittest.TestCase):
	def test(self):
		# testMultimodal
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

		print 'nv = {}'.format(nv)
		print 'n_road_vertices = {} num_vertices(road) = {}'.format(n_road_vertices, tempus.Road.Graph.num_vertices(graph.road()))
		print 'n_pt_vertices = {} num_vertices(pt) = {}'.format(n_pt_vertices, -1) #tempus.Road.num_vertices(graph.road))
		print 'n_pois = {} pois.size() = {}'.format(n_pois, len(graph.pois()))
		print 'num_vertices = {}'.format(tempus.Multimodal.num_vertices(graph))
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