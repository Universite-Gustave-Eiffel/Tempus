/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
 *   Copyright (C) 2012-2014 CEREMA (http://www.cerema.fr/)
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

#include "automaton_lib/automaton.hh"

namespace Tempus {
//
// Implementation of the Dijkstra algorithm
// for a graph and an automaton
template <class RoadGraph,
         class Automaton,
         class VVPredecessorMap,
         class VEPredecessorMap, 
         class EEPredecessorMap, 
         class EVPredecessorMap, 
         class VPotentialMap,
         class EPotentialMap, 
         class WeightMap,
         class PenaltyMap, 
         class RestrictionMap>
std::pair< bool, typename boost::graph_traits<typename Automaton::Graph>::vertex_descriptor > combined_ls_algorithm(
    const RoadGraph& graph,
    const Automaton& automaton,
    typename boost::graph_traits<RoadGraph>::vertex_descriptor start_vertex,
    typename boost::graph_traits<RoadGraph>::vertex_descriptor target_vertex,
    VVPredecessorMap vv_predecessor_map, 
    VEPredecessorMap ve_predecessor_map, 
    EEPredecessorMap ee_predecessor_map, 
    EVPredecessorMap ev_predecessor_map, 
    VPotentialMap vertex_potential_map, 
    EPotentialMap edge_potential_map, 
    WeightMap weight_map, 
    PenaltyMap penalty_map, 
    RestrictionMap simple_restrictions_map
) {
    typedef typename boost::graph_traits<RoadGraph>::vertex_descriptor GVertex;
    typedef typename boost::graph_traits<RoadGraph>::edge_descriptor GEdge;
    typedef typename boost::graph_traits<typename Automaton::Graph>::vertex_descriptor AVertex;

    // label type : graph vertex x automaton state (vertex)
    typedef std::pair<GVertex, AVertex> VertexLabel;
    typedef std::pair<GEdge, AVertex> EdgeLabel; 

    // boost::heap are max heaps, meaning the top() element is the greatest one
    // so the comparison operator must return get(dmap, a) > get(dmap, b)
    // so that top() is always the min distance
    typedef boost::indirect_cmp< VPotentialMap, std::greater<double> > VCmp;
    VCmp vertex_cmp( vertex_potential_map );
    typedef boost::indirect_cmp< EPotentialMap, std::greater<double> > ECmp;
    ECmp edge_cmp( edge_potential_map );

    // priority queue for vertices
    // sort elements by potential_map[v]

#if 0
    // can choose here among types available in boost::heap
    typedef boost::heap::d_ary_heap< Label,
            boost::heap::arity<4>,
            boost::heap::compare< Cmp >,
            boost::heap::mutable_<true> 
    		> EdgeQueue; 
#endif
        
    typedef boost::heap::binomial_heap< VertexLabel,
            boost::heap::compare< VCmp > 
    		>  VertexQueue; 
    
    typedef boost::heap::binomial_heap< EdgeLabel,
            boost::heap::compare< ECmp > 
    		>  EdgeQueue; 
    
    VertexQueue vertex_queue( vertex_cmp ); 
    EdgeQueue edge_queue( edge_cmp );
    
    VertexLabel min_vertex_l = std::make_pair( start_vertex, automaton.initial_state_ ); 
    EdgeLabel min_edge_l; 
    
    unsigned int vertex_iterations = 0; 
    unsigned int edge_iterations = 0; 
    
    put( vertex_potential_map, min_vertex_l, 0.0 );     
    vertex_queue.push( min_vertex_l ); 

    double pi_uq = 0.0;
    double pi_eq = std::numeric_limits<double>::max(); 
    
    while ( ( ! vertex_queue.empty() || ! edge_queue.empty() ) && ( min_vertex_l.first != target_vertex ) ) {
        //std::cout << "Pi(u,q) = " << pi_uq << std::endl; 
        //std::cout << "Pi(e,q) = " << pi_eq << std::endl; 

        if (pi_uq < pi_eq) // From a vertex (vertex minimum potential < edge minimum potential)
        {
            GVertex u = min_vertex_l.first;        
            AVertex q = min_vertex_l.second; 
                        
            vertex_queue.pop();
            vertex_iterations++; 
            //std::cout<< "From (vertex, state) pair (" << graph[u].db_id << ", " << q << "), vertex iterations = " << vertex_iterations << std::endl; 
     
            BGL_FORALL_OUTEDGES_T( u, current_edge, graph, RoadGraph ) { 
            	bool found;
                AVertex s;
                boost::tie( s, found ) = automaton.find_transition( q, current_edge );

                double c = get( weight_map, current_edge ); 
                if ( q != s )  
                    c += get( penalty_map, s ); 
                //std::cout << "Cost + complex penalty = " << c << std::endl; 

                GVertex v = target( current_edge, graph );

                if ( simple_restrictions_map.find(v) == simple_restrictions_map.end() || (v == target_vertex) ) // To a vertex (there is no simple penalized sequence on node v or v is the destination node)
                {
    	            //std::cout << "To vertex " << graph[v].db_id << std::endl; 

                    VertexLabel new_vertex_l = std::make_pair( v, s ); 
                    double pi_vs = get( vertex_potential_map, new_vertex_l );
                    if ( pi_uq + c < pi_vs ) {
                        put( vertex_potential_map, new_vertex_l, pi_uq + c );
                        put( vv_predecessor_map, new_vertex_l, min_vertex_l ); 	
                        vertex_queue.push( new_vertex_l ); 
                    }             
                }
                else // To an edge (there is a simple penalized sequence on node v and v is not the destination node)
                {
                    BGL_FORALL_OUTEDGES_T( v, out_edge, graph, RoadGraph ) { 
                        EdgeLabel new_edge_l = std::make_pair( out_edge, s ); 
                        //std::cout << "To edge (" << graph[source(out_edge, graph)].db_id << ", " << graph[target(out_edge, graph)].db_id << ")" << std::endl; 
                        
                        double pi_es = get( edge_potential_map, new_edge_l ); 
                        std::pair <typename RestrictionMap::iterator, typename RestrictionMap::iterator> simple_penalty_iterator = simple_restrictions_map.equal_range(v); 
                        double simple_penalty=0; 
                        for (typename RestrictionMap::iterator it = simple_penalty_iterator.first; it != simple_penalty_iterator.second ; it++ )
                        {
                            //std::cout << "Arc sequence = (" << graph[it->second.road_edges()[0]].db_id << ", " << graph[it->second.road_edges()[1]].db_id << ")" << std::endl; 
                            if ((it->second.road_edges()[0] == current_edge) && (it->second.road_edges()[1] == out_edge))
                            {		
                                for (Road::Restriction::CostPerTransport::const_iterator penalty_it = it->second.cost_per_transport().begin();
                                     penalty_it != it->second.cost_per_transport().end();
                                     penalty_it++ )
                                {
                                    if (1 && penalty_it->first) simple_penalty = penalty_it->second; 
                                    break; 
                                }
                            } 
                        }
                        //std::cout << "Simple penalty = " << simple_penalty << std::endl; 
                        if ( pi_uq + c < pi_es ) { 
                            put( edge_potential_map, new_edge_l, pi_uq + simple_penalty + c );
                            put( ev_predecessor_map, new_edge_l, min_vertex_l ); 	
                            edge_queue.push( new_edge_l ); 
                        }    
                    }
                }
            }
        } 
        else // From an edge (edge minimum potential <= vertex minimum potential) 
        {
        	GEdge e  = min_edge_l.first; 
        	AVertex q = min_edge_l.second; 

        	edge_queue.pop(); 
        	edge_iterations++; 
        	//std::cout<< "From (edge, state) pair (" << graph[e].db_id << ", " << q << "), edge iterations = " << edge_iterations << std::endl; 

        	GVertex v = target(e, graph); 
        	
    		bool found;
                AVertex s;
                boost::tie( s, found ) = automaton.find_transition( q, e );
                double c = get( weight_map, e ); 
                if ( q != s )  
                    c += get( penalty_map, s ); 
    		//std::cout << "Cost + complex penalty = " << c << std::endl; 
                
                if ( simple_restrictions_map.find(v) == simple_restrictions_map.end() || ( v == target_vertex ) ) // To a vertex (no simple penalized sequence on node v or v is the destination node) => only v will be labelled at this iteration
        	{
                    //std::cout << "To vertex " << graph[v].db_id << std::endl; 
                    VertexLabel new_vertex_l = std::make_pair( v, s ); 
                    double pi_vs = get( vertex_potential_map, new_vertex_l );
                    
                    if ( pi_eq + c < pi_vs ) {
                        put( vertex_potential_map, new_vertex_l, pi_eq + c );
                        put( ve_predecessor_map, new_vertex_l, min_edge_l ); 	
                        vertex_queue.push( new_vertex_l ); 
                    }             
        	} 
                else // To an edge (there is a simple penalized sequence on node v and v is not the destination node) 
                {				
                    BGL_FORALL_OUTEDGES_T( target(e, graph), f, graph, RoadGraph ) { 				
                        EdgeLabel new_edge_l = std::make_pair( f, s ); 			                		
                        //std::cout << "To edge (" << graph[source(f, graph)].db_id << ", " << graph[target(f, graph)].db_id << ")" << std::endl; 
                        double pi_fs = get( edge_potential_map, new_edge_l ); 
            		
                        std::pair <typename RestrictionMap::iterator, typename RestrictionMap::iterator> simple_penalty_iterator = simple_restrictions_map.equal_range(target(e, graph)); 
                        double simple_penalty=0; 
                        for (typename RestrictionMap::iterator it = simple_penalty_iterator.first; it != simple_penalty_iterator.second ; it++ )
                        {
                            //std::cout << "Arc sequence = (" << graph[it->second.road_edges()[0]].db_id << ", " << graph[it->second.road_edges()[1]].db_id << ")" << std::endl; 
                            if ((it->second.road_edges()[0] == e) && (it->second.road_edges()[1] == f))
                            {		
                                for (Road::Restriction::CostPerTransport::const_iterator penalty_it = it->second.cost_per_transport().begin();
                                     penalty_it != it->second.cost_per_transport().end();
                                     penalty_it++ )
                                {
                                    if (1 && penalty_it->first) simple_penalty = penalty_it->second; 
                                    break; 
                                }
                            } 
                        }
            		//std::cout << "Simple penalty = " << simple_penalty<< std::endl; 
                        if ( pi_eq + c < pi_fs ) { 
                            put( edge_potential_map, new_edge_l, pi_eq + c + simple_penalty );
                            put( ee_predecessor_map, new_edge_l, min_edge_l ); 
                            edge_queue.push( new_edge_l ); 
                        }    
                    }
                }
        }
        
        if (!vertex_queue.empty()) 
        {
            min_vertex_l = vertex_queue.top();
            pi_uq = get( vertex_potential_map, min_vertex_l ); 
            //std::cout << "Next possible vertex " << min_vertex_l.first << std::endl; 
        }
        else pi_uq = std::numeric_limits<double>::max(); 
        if (!edge_queue.empty()) 
        {
            min_edge_l = edge_queue.top(); 
            pi_eq = get( edge_potential_map, min_edge_l );         
            //std::cout << "Next possible edge " << min_edge_l.first << std::endl; 
        }
        else pi_eq = std::numeric_limits<double>::max(); 
    } // while     
    
    if ( vertex_queue.empty() && edge_queue.empty() ) return std::make_pair(false, automaton.initial_state_); 
    else return std::make_pair(true, min_vertex_l.second); 
}

}
