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

// Path algorithms for combined graphs with homogen labeling strategy

#include <boost/pending/indirect_cmp.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include <boost/heap/binomial_heap.hpp>

//
// Implementation of the Dijkstra algorithm (label-setting)
// for a graph and an automaton
template < class Graph,
         class Automaton,
         class PredecessorMap,
         class PotentialMap,
         class WeightMap,
         class PenaltyMap, 
         class Visitor >
void combined_ls_algorithm( const Graph& graph, const Automaton& automaton,
							typename boost::graph_traits<Graph>::vertex_descriptor start_vertex, PredecessorMap& predecessor_map,
							PotentialMap& potential_map, WeightMap weight_map, PenaltyMap penalty_map, Visitor vis ) {
	
    typedef typename boost::graph_traits<Graph>::vertex_descriptor GVertex;
    typedef typename boost::graph_traits<typename Automaton::Graph>::vertex_descriptor AVertex;
	typedef std::pair< GVertex, AVertex> Object; 
    
    
    // boost::heap are max heaps, meaning the top() element is the greatest one
    // so the comparison operator must return get(dmap, a) > get(dmap, b)
    // so that top() is always the min distance
    typedef boost::indirect_cmp< PotentialMap, std::greater<double> > Cmp;
    Cmp cmp( potential_map ); 


    // we can choose here among types available in boost::heap
    typedef boost::heap::d_ary_heap< Object, boost::heap::arity<4>,boost::heap::compare< Cmp >,boost::heap::mutable_<true> > VertexQueue; 
    
    VertexQueue vertex_queue( cmp );  
    
    Object min_o; 
    min_o.first = start_vertex; 
    min_o.second = automaton.initial_state_; 
    
    put( potential_map, min_o, 0.0 ); 
    vis.discover_vertex( min_o.first, graph ); 
    vertex_queue.push( min_o ); 

    while ( !vertex_queue.empty() ) {
        
    	vis.examine_vertex( min_o.first, graph ); 
        
        double min_pi= get( potential_map, min_o );
        
        vertex_queue.pop();

        BGL_FORALL_OUTEDGES_T( min_o.first, current_edge, graph, Graph ) { 
        	vis.examine_edge( current_edge, graph ); 
        	
            GVertex v = target( current_edge, graph );
            bool found;
            AVertex s;
            boost::tie( s, found )= find_transition( automaton.automaton_graph_, min_o.second, current_edge.road_edge );
            Object new_o; 
            new_o.first = v; 
            new_o.second = s; 
            double new_pi = get( potential_map, new_o ); 
            
            double c = get( weight_map, current_edge ); 
            if ( min_o.second != new_o.second )  
                c += get( penalty_map, s ); 
            
            if ( min_pi + c < new_pi ) {
            	vis.edge_relaxed( current_edge, graph ); 
                put( potential_map, new_o, min_pi + c );
                put( predecessor_map, new_o, min_o ); 
                
                vis.discover_vertex( new_o.first, graph ); 
                vertex_queue.push( new_o ); 
            }
            else vis.edge_not_relaxed( current_edge, graph ); 
        }
        vis.finish_vertex( min_o.first, graph ); 
        
        min_o = vertex_queue.top(); 
    } // while 
}

// Implementation of the Bellman algorithm (label-correcting)
// for a graph and an automaton
template < class Graph,
         class Automaton,
         class PredecessorMap,
         class PotentialMap,
         class WeightMap,
         class PenaltyMap,
         class Visitor >
void combined_lc_algorithm( const Graph& graph, const Automaton& automaton,
							typename boost::graph_traits<Graph>::vertex_descriptor start_vertex, PredecessorMap predecessor_map, 
							PotentialMap potential_map, WeightMap weight_map, PenaltyMap penalty_map, Visitor vis ) {
    
	typedef typename boost::graph_traits<Graph>::vertex_descriptor GVertex;
    typedef typename boost::graph_traits<typename Automaton::Graph>::vertex_descriptor AVertex;

    // object type : graph vertex x automaton state (vertex)
	typedef std::pair< GVertex, AVertex> Object; 
    
    typedef std::list< Object >  VertexQueue; 
    VertexQueue vertex_queue; 

    Object min_o; 
    min_o.first = start_vertex ; 
    min_o.second = automaton.initial_state_ ;
    
    put( potential_map, min_o, 0.0 );
    vertex_queue.push_back( min_o ); 

    while ( !vertex_queue.empty() )
    {
    	Object current_o = vertex_queue.front(); 
    	vertex_queue.pop_front(); 
    	
    	double current_pi = get( potential_map, current_o );
    	
		BGL_FORALL_OUTEDGES_T( current_o.first, current_edge, graph, Graph ) { 
			vis.examine_edge( current_edge, graph ); 
			GVertex v = target( current_edge, graph );
			bool found;
			AVertex s;
			boost::tie( s, found )= find_transition( automaton.automaton_graph_, current_o.second, current_edge.road_edge );
			Object new_o; 
			new_o.first = v; 
			new_o.second = s; 
			double new_pi = get( potential_map, new_o );
			
			double c = get( weight_map, current_edge ); 
			if ( new_o.second != current_o.second )  
				c += get( penalty_map, new_o.second ); 
			if ( current_pi + c < new_pi ) {
				put( potential_map, new_o, current_pi + c );
				put( predecessor_map, new_o, current_o ); 
			
			if ( std::find(vertex_queue.begin(), vertex_queue.end(), new_o ) == vertex_queue.end() )
				vertex_queue.push_back( new_o ); 
			}
    	}
    } 
}



