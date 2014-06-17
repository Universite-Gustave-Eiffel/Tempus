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

#include <boost/format.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <map>

#include "utils/associative_property_map_default_value.hh"

#include "plugin.hh"
#include "road_graph.hh"
#include "db.hh"
#include "pgsql_importer.hh"
#include "utils/timer.hh"
#include "utils/graph_db_link.hh"
#include "utils/field_property_accessor.hh"
#include "utils/function_property_accessor.hh"

#include "automaton.hh"
#include "combined_algorithms.hh"

#define PEDEST_SPEED 1 // Pedestrian speed in m/sec
#define CYCLE_SPEED 5 // Cycle speed in m/sec

using namespace std;

namespace Tempus {

    // Travel time function of the road graph (in minutes) 
    class TravelTimeCalculator
    {
    public:
        TravelTimeCalculator( int mode ) : mode_(mode) {} // constructeur
        		
        double operator() ( Road::Graph& graph, Road::Edge& e ) { 
            if ( (graph[e].transport_type & mode_) == 0 )
                return std::numeric_limits<double>::infinity();
            else if ( mode_ == 1 || mode_ == 256 ) // Car
                return graph[e].length /  (graph[e].car_average_speed / 0.06); 
            else if ( mode_ == 2 ) // Pedestrial
                return graph[e].length/(PEDEST_SPEED*60); 
            else if ( mode_ == 4 || mode_ == 128 ) // Bicycle
                return graph[e].length/(CYCLE_SPEED*60); 
            else return std::numeric_limits<double>::infinity(); 
        }
    private:
        unsigned int mode_; // transport mode
    }; 
        
    class DistanceCalculator
    {
    public:
        DistanceCalculator( int mode ) : mode_(mode) {} 
        	
        double operator() ( Road::Graph& graph, Road::Edge& e ) {
            if ( (graph[e].transport_type & mode_) == 0 ) {
                return std::numeric_limits<double>::infinity();
            }
            return graph[e].length;
        }
    private:
        unsigned int mode_; // transport mode
    };
    	
class HeterogenTMRRoadPlugin : public Plugin {

public:
    static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option("sequence_description", "Nodes (1) or arcs (2)", 1 ); 
        odl.declare_option("labelling_strategy", "Nodes (0), arcs (1) or mixt (2)", 0); 
        odl.declare_option("combination_strategy", "Algorithm (0) or graph adaptator (1)", 0); 
        odl.declare_option("trace_vertex", "Trace vertex traversal", false);
        odl.declare_option("prepare_result", "Prepare result", true);
        return odl;
    }

    HeterogenTMRRoadPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
        OptionDescriptionList odl( option_descriptions() );
        odl.set_options_default_value( this );
    }

    virtual ~HeterogenTMRRoadPlugin() {
    }

protected:
    bool trace_vertex_;
    bool prepare_result_;
    short int sequence_description; 
    static Automaton<Road::Edge> automaton_;
    static std::multimap<Road::Vertex, Road::Restriction> simple_restrictions_;
    
    // exception returned to shortcut Dijkstra
    struct path_found_exception {};

    Road::Vertex source_, destination_;

public:
    static void post_build() {
    	Road::Graph& road_graph = Application::instance()->graph().road;
    	
    	// Building automaton 
		PQImporter psql( Application::instance()->db_options() ); 
		Road::Restrictions restrictions = psql.import_turn_restrictions( road_graph );
		std::cout << "turn restrictions imported" << std::endl; 
		
		// Separating restrictions between simple (2 arcs) and complex (3 arcs or more) 
	    Road::Restrictions complex_restrictions( road_graph ); 
	    for ( std::list<Road::Restriction>::const_iterator it=restrictions.restrictions().begin() ; it!=restrictions.restrictions().end() ; it++ ) {
			if ( it->road_edges().size() == 2 ) simple_restrictions_.insert(std::pair<Road::Vertex, Road::Restriction>(target(it->road_edges()[0], road_graph), *it ) ); 
			else complex_restrictions.add_restriction( *it );
		} 
		
		automaton_.build_graph( complex_restrictions ) ;
		std::cout << "automaton built" << std::endl; 
		
		std::ofstream ofs("../out/complex_movements_edge_automaton.dot"); // output to graphviz
		Automaton<Road::Edge>::NodeWriter nodeWriter( automaton_.automaton_graph_ );
		Automaton<Road::Edge>::ArcWriter arcWriter( automaton_.automaton_graph_, road_graph );
		boost::write_graphviz( ofs, automaton_.automaton_graph_, nodeWriter, arcWriter);
		std::cout << "automaton exported to graphviz" << std::endl; 
    }

    virtual void pre_process( Request& request ) {   	    	
    	
    	Road::Graph& road_graph = Application::instance()->graph().road; 
    			
    	// Recording request
		/*if ( ( request.optimizing_criteria[0] != CostDistance ) ) {
		  throw std::invalid_argument( "Unsupported optimizing criterion" );
		} */
			
		request_ = request;
		source_ = request.origin;
		destination_ = request.destination();
				
		REQUIRE( request.check_consistency() );
		REQUIRE( vertex_exists( request.origin, graph_.road ) );
		REQUIRE( vertex_exists( request.destination(), graph_.road ) );
		
		std::cout << "source: " << road_graph[source_].db_id << " target: " << road_graph[destination_].db_id << std::endl;
		
		// Recording options 
		get_option( "trace_vertex", trace_vertex_ );
		get_option( "prepare_result", prepare_result_ );
		
		// Deleting result
		result_.clear(); 
    }
    
    
    virtual void process() {
    	// Copy road graph
        Road::Graph& road_graph = graph_.road;
        
        Timer timer;

        // The Label type which is a pair of (graph vertex, automaton transition)
        typedef std::pair< Road::Vertex, Automaton<Road::Edge>::Graph::vertex_descriptor > VertexLabel;
        typedef std::pair< Road::Edge, Automaton<Road::Edge>::Graph::vertex_descriptor > EdgeLabel; 

        // Predecessor map
        typedef std::map<VertexLabel, VertexLabel> VertexVertexPredecessorMap;
        typedef std::map<VertexLabel, EdgeLabel> VertexEdgePredecessorMap; 
        typedef std::map<EdgeLabel, EdgeLabel> EdgeEdgePredecessorMap; 
        typedef std::map<EdgeLabel, VertexLabel> EdgeVertexPredecessorMap; 
        
        VertexVertexPredecessorMap vv_predecessor_map;
        boost::associative_property_map< VertexVertexPredecessorMap > vv_predecessor_pmap( vv_predecessor_map );  // adaptation to a property map
        VertexEdgePredecessorMap ve_predecessor_map;
        boost::associative_property_map< VertexEdgePredecessorMap > ve_predecessor_pmap( ve_predecessor_map );  // adaptation to a property map
        EdgeEdgePredecessorMap ee_predecessor_map;
        boost::associative_property_map< EdgeEdgePredecessorMap > ee_predecessor_pmap( ee_predecessor_map );  // adaptation to a property map
        EdgeVertexPredecessorMap ev_predecessor_map;
        boost::associative_property_map< EdgeVertexPredecessorMap > ev_predecessor_pmap( ev_predecessor_map );  // adaptation to a property map
        
        
        ///// Maps storing the calculated optimisation criterion 
        typedef std::map<VertexLabel, double> VertexPotentialMap;
        typedef std::map<EdgeLabel, double> EdgePotentialMap; 
        
        // Path travel time map
        VertexPotentialMap vertex_path_tt_map;
        associative_property_map_default_value< VertexPotentialMap > vertex_path_tt_pmap( vertex_path_tt_map, std::numeric_limits<double>::max() ); // adaptation to a property map : infinite default value 
        EdgePotentialMap edge_path_tt_map; 
        associative_property_map_default_value< EdgePotentialMap > edge_path_tt_pmap( edge_path_tt_map, std::numeric_limits<double>::max() ); 
        
        // Path distance map
        VertexPotentialMap vertex_path_dist_map;
        associative_property_map_default_value< VertexPotentialMap > vertex_path_dist_pmap( vertex_path_dist_map, std::numeric_limits<double>::max() ); // adaptation to a property map : infinite default value 
        EdgePotentialMap edge_path_dist_map;
        associative_property_map_default_value< EdgePotentialMap > edge_path_dist_pmap( edge_path_dist_map, std::numeric_limits<double>::max() ); // adaptation to a property map : infinite default value 
        
        ///// Maps storing arc and node weights
        // Node penalty map
        Automaton<Road::Edge>::PenaltyCalculator penalty_calculator(1); 
        Tempus::FunctionPropertyAccessor< Automaton< Road::Edge >::Graph,
                                   boost::vertex_property_tag,
                                   double,
                                   Automaton< Road::Edge >::PenaltyCalculator > vertex_penalty_pmap( automaton_.automaton_graph_, penalty_calculator ); 
        // Arc travel time map
        TravelTimeCalculator travel_time_calculator(1); 
        Tempus::FunctionPropertyAccessor< Road::Graph,  
                                          boost::edge_property_tag, 
                                          double, 
                                          TravelTimeCalculator > edge_tt_pmap( road_graph, travel_time_calculator ); 

        
        // Arc distance map
        DistanceCalculator distance_calculator(1); 
		Tempus::FunctionPropertyAccessor< Road::Graph,  
									boost::edge_property_tag, 
									double, 
									DistanceCalculator > edge_dist_pmap( road_graph, distance_calculator ); 

        // Case of an automaton labeled with edges of the road graph 
        typedef typename Automaton<Road::Edge>::Node AutomatonNode; 
        
        int combination_strategy;         
        get_option( "combination_strategy", combination_strategy );

        std::pair<bool, Automaton<Road::Edge>::Vertex> res; 
        
        if ( combination_strategy == 0 ) {
        	
        	std::cout << "combined dijkstra" << std::endl;

        	res = combined_ls_algorithm( road_graph, 
        						   automaton_, 
        						   source_, 
        						   destination_, 
								   vv_predecessor_pmap,
								   ve_predecessor_pmap, 
								   ee_predecessor_pmap, 
								   ev_predecessor_pmap, 
								   vertex_path_tt_pmap, 
								   edge_path_tt_pmap, 
        						   edge_tt_pmap, 
        						   vertex_penalty_pmap, 
        						   simple_restrictions_);  
        }
        
        
#if 0
        else if ( combination_strategy == 1 ) {
        	// the combined graph type
        	typedef CombinedGraphAdaptor<Road::Graph, typename Automaton<Road::Edge>::Graph > CGraph;
        	CGraph cgraph( road_graph, automaton_.automaton_graph_ );
        	// the combined weight map made of a weight map on the road graph and a penalty map
			typedef CombinedWeightMap< CGraph, WeightMap, PenaltyMap > CombinedWeightMap;
			CombinedWeightMap wmap( cgraph, weight_map, penalty_map );
        }


	  // manually set the distance of the source_ to 0.0 (no init here)
	  typename Automaton<Road::Edge>::Vertex q0 = automaton_.initial_state_;
	  put( distance_pmap, std::make_pair( source_, q0 ), 0.0 );

	  // we call the _no_init variant here,
	  // otherwise the potential_map (and predecessor_map) would be initialized on a potentially big
	  // number of vertices
	  boost::dijkstra_shortest_paths_no_color_map_no_init( cgraph,
							       std::make_pair( source_, q0 ),
							       predecessor_pmap,
							       distance_pmap,
							       wmap,
							       get( boost::vertex_index, cgraph ),
							       std::less<double>(),
							       boost::closed_plus<double>(),
							       std::numeric_limits<double>::max(),
							       0.0,
							       boost::dijkstra_visitor<boost::null_visitor>()
                                   ); 
        }
#endif

        std::list<Road::Vertex> path;
        std::cout << "source: " << road_graph[source_].db_id << " target: " << road_graph[destination_].db_id << std::endl;
        
        if ( res.first ) // A path has been found, it will be now reconstructed from the predecessor maps 
        {
        	VertexLabel current_vertex_l = std::make_pair( destination_, res.second ); 
        	EdgeLabel current_edge_l ; 
        	bool current_vertex = true ; 
			std::cout << road_graph[current_vertex_l.first].db_id << std::endl; 

        	while ( current_vertex_l.first != source_ ) 
        	{
    			std::cout << road_graph[current_vertex_l.first].db_id << std::endl; 

        		if ( current_vertex ) // From vertex
    			{
    				std::cout << road_graph[current_vertex_l.first].db_id << std::endl; 
    				path.push_front( current_vertex_l.first ); 
					// Find predecessor : vertex or edge 
					if ( vv_predecessor_map.find(current_vertex_l) != vv_predecessor_map.end() ) // Vertex predecessor
					{
						current_vertex_l = vv_predecessor_map[ current_vertex_l ]; 
						current_vertex = true; 
					}
					else if ( ve_predecessor_map.find(current_vertex_l) != ve_predecessor_map.end() ) // Edge predecessor
					{
						current_edge_l = ve_predecessor_map[ current_vertex_l ];
						current_vertex = false; 
					}
    			}
    			else // From edge 
    			{
    				std::cout << road_graph[ source(current_edge_l.first, road_graph) ].db_id << std::endl; 
					path.push_front( source(current_edge_l.first, road_graph) ); 
					// Find predecessor : vertex or edge
					if ( ev_predecessor_map.find(current_edge_l) != ev_predecessor_map.end() ) // Vertex predecessor
					{
						current_vertex_l = ev_predecessor_map[ current_edge_l ]; 
						current_vertex = true; 
					}
					else if ( ee_predecessor_map.find(current_edge_l) != ee_predecessor_map.end() ) // Edge predecessor
					{
						current_edge_l = ee_predecessor_map[ current_edge_l ];
						current_vertex = false; 
					}
    			} 
        	}
        	path.push_front( current_vertex_l.first ); // adding source node to the path 
        	
        	metrics_[ "time_s" ] = timer.elapsed();
        	        	        
			result_.push_back( Roadmap() );
			Roadmap& roadmap = result_.back();

			bool first_loop = true;
			Road::Vertex previous;

			for ( std::list<Road::Vertex>::iterator it = path.begin(); it != path.end(); it++ ) {
				Road::Vertex v = *it;

				// User-oriented roadmap
				if ( first_loop ) {
					previous = v;
					first_loop = false;
					continue;
				}
				
				// Find an edge, based on a source and destination vertex
				Road::Edge e;
				bool found = false; 
				boost::tie( e, found ) = boost::edge( previous, v, road_graph );

				if ( !found ) {
					continue;
				}

				Roadmap::RoadStep* step = new Roadmap::RoadStep();
				roadmap.steps.push_back( step );
				step->road_edge = e; 
				
				step->costs[CostDistance] += get(edge_dist_pmap, e);
				step->costs[CostDuration] += get(edge_tt_pmap, e); 
				roadmap.total_costs[CostDistance] += get(edge_dist_pmap, e);
				roadmap.total_costs[CostDuration] += get(edge_tt_pmap, e); 
				
				previous = v;
			} 
        }
    }
};

Automaton<Road::Edge> HeterogenTMRRoadPlugin::automaton_ = Automaton<Road::Edge>();
std::multimap<Road::Vertex, Road::Restriction> HeterogenTMRRoadPlugin::simple_restrictions_ = std::multimap<Road::Vertex, Road::Restriction>(); 

}

DECLARE_TEMPUS_PLUGIN( "heterogen_tmr_road_plugin", Tempus::HeterogenTMRRoadPlugin )



