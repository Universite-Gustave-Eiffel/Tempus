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

// Dijkstra for combined graphs

#include <boost/pending/indirect_cmp.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include <boost/heap/binomial_heap.hpp>

//
// Implementation of the Dijkstra algorithm
// for a graph and an automaton
template <class Graph,
         class Automaton,
         class DistanceMap,
         class PredecessorMap,
         class WeightMap,
         class PenaltyMap>
void combined_dijkstra(
    const Graph& graph,
    const Automaton& automaton,
    PenaltyMap penalty_map,
    typename boost::graph_traits<Graph>::vertex_descriptor start_vertex,
    typename boost::graph_traits<Graph>::vertex_descriptor target_vertex,
    PredecessorMap predecessor_map,
    DistanceMap distance_map,
    WeightMap weight_map
) {
    typedef typename boost::graph_traits<Graph>::vertex_descriptor GVertex;
    typedef typename boost::graph_traits<Automaton>::vertex_descriptor AVertex;

    // label type : graph vertex x automaton state (vertex)
    typedef std::pair<GVertex, AVertex> Label;

    // boost::heap are max heaps, meaning the top() element is the greatest one
    // so the comparison operator must return get(dmap, a) > get(dmap, b)
    // so that top() is always the min distance
    typedef boost::indirect_cmp< DistanceMap, std::greater<double> > Cmp;
    Cmp cmp( distance_map );

    // priority queue for vertices
    // sort elements by distance_map[v]

#if 0
    // we can choose here among types available in boost::heap
    typedef boost::heap::d_ary_heap< Label,
            boost::heap::arity<4>,
            boost::heap::compare< Cmp >,
            boost::heap::mutable_<true>
            >
#endif
            typedef boost::heap::binomial_heap< Label,
            boost::heap::compare< Cmp >
            >
            VertexQueue;

    VertexQueue vertex_queue( cmp );


    AVertex q0 = *vertices( automaton ).first;
    Label l0 = std::make_pair( start_vertex, q0 );
    put( distance_map, l0, 0.0 );
    vertex_queue.push( l0 );

    while ( !vertex_queue.empty() ) {
        for ( typename VertexQueue::ordered_iterator it = vertex_queue.ordered_begin(); it != vertex_queue.ordered_end(); ++it ) {
            COUT << graph[it->first].db_id << ", " << it->second << " dist: " << get( distance_map, *it ) << std::endl;
        }

        Label min_l = vertex_queue.top();

        GVertex u = min_l.first;

        if ( u == target_vertex ) {
            break;
        }

        AVertex q = min_l.second;

        COUT << "u: " << graph[u].db_id << ", " << q << std::endl;

        vertex_queue.pop();

        //double min_distance = get( distance_map, min_l );

        typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
        BGL_FORALL_OUTEDGES_T( u, current_edge, graph, Graph ) {

            GVertex v = target( current_edge, graph );
            COUT << "v: " << graph[v].db_id << std::endl;

            // is an automaton transition has been found ?
            AVertex qp = find_transition( automaton, q, u, v );

            double c = get( weight_map, current_edge );

            if ( q != qp ) {
                double p = get( penalty_map, q );
                COUT << "penalty: " << p << std::endl;
                c += p;
            }

            Label new_l = std::make_pair( v, qp );
            double pi_uq = get( distance_map, min_l );
            double pi_vqp = get( distance_map, new_l );

            if ( pi_uq + c < pi_vqp ) {
                put( distance_map, new_l, pi_uq + c );
                put( predecessor_map, new_l, min_l );
                COUT << "push " << graph[new_l.first].db_id << ", " << new_l.second << " dist: " << ( pi_uq + c ) << std::endl;

                vertex_queue.push( new_l );

            }
        }

        COUT << "loop" << std::endl;
    }

    COUT << "end" << std::endl;
}

