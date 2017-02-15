"""
/**
 *   Copyright (C) 2012-2017 Oslandia <infos@oslandia.com>
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

"""
/**
 *   Copyright (C) 2012-2017 Oslandia <infos@oslandia.com>
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

# This is an example of a Tempus plugin written in Python
# See the standalone Python application to test it

import pytempus as tempus

from graph_tool.all import *

class EuclidianHeuristic():
    def __init__(self, road_graph, road_vertex_target):
        self.road_graph = road_graph
        print ('{} - {}'.format(road_graph, road_vertex_target))
        # vertex = road_graph.road_vertex_from_id(target)
        p3d = road_graph[road_vertex_target].coordinates
        self.destination = tempus.Point2D(p3d.x, p3d.y)

    def __call__(self, road_vertex):
        p3d = self.road_graph[road_vertex].coordinates
        return tempus.distance2(self.destination, tempus.Point2D(p3d.x, p3d.y))


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

        g = self.plugin.graph_
        road_graph = self.plugin.g.road()

        source = self.plugin.g.road_vertex_from_id(request.origin)
        target = self.plugin.g.road_vertex_from_id(request.destination)

        h = EuclidianHeuristic(road_graph, target)

        v = GoalVisitor(target)

        dist, pred = astar_search(g, source, self.plugin.const_tt, visitor=v, heuristic=h)

        path = []

        v = target
        while v != source:
            path += [v]
            if v == pred[v]:
                raise RuntimeException('no path found')
            v = pred[v]

        path += [source]

        print '->'.join(str(i) for i in path)

        path.reverse()

        prev = path[0]
        roadmap = tempus.Roadmap()
        for v in path[1:]:
            step = roadmap.RoadStep()
            edge = tempus.edge(prev, v, road_graph)
            if edge is None:
                print("Couldnt find edge {}->{}".format(prev, v))
                prev = v
                continue

            prev = v
            step.road_edge_id = road_graph[edge].db_id;
            step.set_cost(tempus.CostId.CostDistance, road_graph[edge].length)
            # step.set_cost(tempus.CostId.CostDuration, self.plugin.const_tt[ed])
            step.set_cost(tempus.CostId.CostDuration, road_graph[edge].length / (3.6 * 1000 / 60.0))
            roadmap.add_step(step)

        r = tempus.ResultElement(roadmap)
        return [r]


class AStar(tempus.Plugin):
    def __init__(self, progression, options):
        tempus.Plugin.__init__(self, 'py_astar_road_plugin')
        # load data
        self.g = tempus.load_routing_data("multimodal_graph", progression, options)
        self.traffic_rule = tempus.TransportModeTrafficRule.TrafficRulePedestrian

        road_graph = self.g.road()
        print road_graph
        nv = road_graph.num_vertices()
        ne = road_graph.num_edges()
        print "num: vertices=", nv, "edges=", ne


        self.graph_ = Graph()
        self.graph_.add_vertex(nv)
        self.distance = self.graph_.new_edge_property("double")
        self.const_tt = self.graph_.new_edge_property("double")

        print 'Creating {} edges'.format(ne)
        for i in range(0, ne):
            edge = road_graph.edge_from_index(i)
            e = self.graph_.add_edge(road_graph.source(edge), road_graph.target(edge))

            ed = road_graph[edge]
            self.const_tt[e] = ed.length / (3.6 * 1000 / 60) if ed.traffic_rules & self.traffic_rule else float('inf')

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
    return tempus.Plugin.Capabilities()

def name():
    return 'py_astar_road_plugin'
