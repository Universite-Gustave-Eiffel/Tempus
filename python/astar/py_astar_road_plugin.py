import tempus

from graph_tool.all import *

class EuclidianHeuristic():
    def __init__(self, graph, target):
        self.graph = graph
        # TODO self.target = graph.vertex(target).coordinates()

    def __call__(self, v):
        return 1.0


class GoalVisitor(AStarVisitor):
    def __init__(self, target):
        self.target = target

    def examine_vertex(self, u):
        if self.target == u:
            raise StopSearch()

class PyAStarRequest(tempus.PluginRequest):
    def __init__(self, plugin, options):
        tempus.PluginRequest.__init__(self, plugin, options)
        self.plugin = plugin

    def process(self, request):
        print 'process!'
        r = tempus.ResultElement()

        g = self.plugin.graph_
        source = self.plugin.g.road_vertex_from_id(request.origin)
        target = self.plugin.g.road_vertex_from_id(request.destination)

        print source
        print target

        h = EuclidianHeuristic(g, target)

        v = GoalVisitor(target)

        dist, pred = astar_search(g, source, self.plugin.cost, visitor=v, heuristic=h)

        print dist
        print pred

        path = []

        v = target
        while v != source:
            path += [v]
            if v == pred[v]:
                raise RuntimeException('no path found')
            v = pred[v]

        path += [source]
        path.reverse()
        print path
        return [r]


class AStar(tempus.Plugin):
    def __init__(self, progression, options):
        tempus.Plugin.__init__(self, 'py_astar_road_plugin')
        # load data
        self.g = tempus.load_routing_data("multimodal_graph", progression, options)

        road_graph = self.g.road()
        print road_graph
        nv = road_graph.num_vertices()
        ne = road_graph.num_edges()
        print "num: vertices=", nv, "edges=", ne


        self.graph_ = Graph()
        self.graph_.add_vertex(nv)
        self.distance = self.graph_.new_edge_property("double")
        self.cost = self.graph_.new_edge_property("double")

        print 'Creating {} edges'.format(ne)
        for i in range(0, ne):
            edge = road_graph.edge_from_index(i)
            e = self.graph_.add_edge(road_graph.source(edge), road_graph.target(edge))

        #   # todo
        #   self.distance[e] = 1.0
            self.cost[e] = 1.0

        print 'Done'

    def routing_data(self):
        return self.graph_

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
