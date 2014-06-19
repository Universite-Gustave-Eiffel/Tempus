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

#include <boost/format.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <map>

#include "utils/associative_property_map_default_value.hh"

#include "plugin.hh"
#include "road_graph.hh"
#include "db.hh" 
#include "automaton.hh" 
#include "pgsql_importer.hh"
#include "utils/graph_db_link.hh"
#include "utils/timer.hh" 
#include "utils/field_property_accessor.hh"
#include "utils/function_property_accessor.hh"

#include "combined_algorithms.hh"
#include "travel_time_calculator.hh"

using namespace std;

namespace Tempus {

/*struct Object {
	Multimodal::Graph::vertex_descriptor vertex; 
	Automaton<Road::Edge>::Graph::vertex_descriptor state;
}; */

typedef std::pair< Multimodal::Graph::vertex_descriptor, Automaton<Road::Edge>::Graph::vertex_descriptor> Object; 
typedef std::list< Object > Path; 
typedef std::map< Object, double > PotentialMap; 

class HomogenTMRRoadPlugin : public Plugin {
public:
	static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option("verbose", "Verbose", false);
        odl.declare_option("combination", "Algorithm (0) or graph (1) adaptator", 0);
        odl.declare_option("labeling", "Object-setting (0) or Object-correcting (1) algorithm", 0); 
        odl.declare_option("walking_speed", "Walking speed (km/h)", 3.6); 
        odl.declare_option("cycling_speed", "Cycling speed (km/h)", 12); 
        return odl; 
    }
	
    HomogenTMRRoadPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
        OptionDescriptionList odl( option_descriptions() );
        odl.set_options_default_value( this );
    }

    virtual ~HomogenTMRRoadPlugin() { }

protected:    
    // Plugin options
	bool verbose_; // verbose vertex and edge traversal 
    int combination_; 
    int labeling_; 
    double walking_speed_; 
    double cycling_speed_; 
    // Plugin metrics ( int, double or bool types possible )
	int examined_edges_; // Number of edges examined
	int examined_vertices_ ; // Number of vertices examined 
	double time_; // Time elapsed for pre_process() and process() 
	double time_algo_; // Time elapsed for process() 
	// Other variables
    struct path_found_exception {}; // exception returned to shortcut Dijkstra
    Multimodal::Vertex destination_; // Current request destination 
    bool found_; // Indicates if the path has been found 
    PotentialMap potential_map_; // Map giving the Object values (costs of the shortest paths) 
    static Automaton<Road::Edge> automaton_; 

public:
    static void post_build() { // The penalized movements automaton is built
    	Road::Graph& road_graph = Application::instance()->graph().road;
        
		PQImporter psql( Application::instance()->db_options() ); 
		Road::Restrictions restrictions = psql.import_turn_restrictions( road_graph );
		//std::cout << "turn restrictions imported" << std::endl;
		
		automaton_.build_graph( restrictions ) ;
		std::cout << "Automaton built" << std::endl; 
		
		std::ofstream ofs("../out/all_movements_edge_automaton.dot"); // output to graphviz
		Automaton<Road::Edge>::NodeWriter nodeWriter( automaton_.automaton_graph_ );
		Automaton<Road::Edge>::ArcWriter arcWriter( automaton_.automaton_graph_, road_graph );
		boost::write_graphviz( ofs, automaton_.automaton_graph_, nodeWriter, arcWriter);
		std::cout << "Automaton exported to graphviz" << std::endl; 
    }

    virtual void pre_process( Request& request ) {  			
    	Timer timer; 
    	time_=0; 
    	for ( size_t i = 0; i < request.optimizing_criteria.size(); ++i ) {
			REQUIRE( request.optimizing_criteria[i] == CostDuration );
		}
		
    	found_ = false; 
		request_ = request;
		destination_ = Multimodal::Vertex( &graph_.road, request_.steps[0].destination ); 
				
		REQUIRE( request.check_consistency() );
		REQUIRE( vertex_exists( request.origin, graph_.road ) );
		REQUIRE( vertex_exists( destination_, graph_.road ) );
		
		
		// Recording options 
		get_option( "verbose", verbose_ );
		get_option( "combination", combination_ );
		get_option( "labeling", labeling_ ); 
		get_option( "walking_speed", walking_speed_ ); 
		get_option( "cycling_speed", cycling_speed_ ); 
		
		// Deleting result
		result_.clear(); 
		
        time_+=timer.elapsed(); 
    }
    
    virtual void process() {
        examined_edges_=0;
        examined_vertices_=0; 
        
        Timer timer; 
    	time_algo_=0; 
    	
        // The Object type which is a pair of (graph vertex, automaton transition)

        // Predecessor map
        typedef std::map<Object, Object> PredecessorMap;
        PredecessorMap predecessor_map;
        boost::associative_property_map< PredecessorMap > predecessor_pmap( predecessor_map );  // adaptation to a property map
        
        associative_property_map_default_value< PotentialMap > potential_pmap( potential_map_, std::numeric_limits<double>::max() ); // adaptation to a property map : infinite default value 
        
        ///// Maps storing arc and node weights
        // Node penalty map
        Automaton<Road::Edge>::PenaltyCalculator penalty_calculator(1); 
        typedef Tempus::FunctionPropertyAccessor< Automaton< Road::Edge >::Graph,
                                   boost::vertex_property_tag,
                                   double, 
                                   Automaton< Road::Edge >::PenaltyCalculator > PenaltyMap; 
        PenaltyMap node_penalty_pmap( automaton_.automaton_graph_, penalty_calculator ); 
        
        // Arc travel time map
        TravelTimeCalculator travel_time_calculator(1, walking_speed_, cycling_speed_); 
        typedef Tempus::FunctionPropertyAccessor< Multimodal::Graph,  
        							boost::edge_property_tag, 
        							double, 
        							TravelTimeCalculator > TravelTimeMap; 
        TravelTimeMap tt_pmap( graph_, travel_time_calculator ); 
        
        double departure_time = request_.steps[0].constraint.date_time.time_of_day().total_seconds()/60; 
        Object o; 
        o.first = Multimodal::Vertex( &graph_.road, request_.origin ); 
        o.second = 0; 
        potential_map_[ o ] = departure_time;  
        
        Tempus::PluginGraphVisitor vis ( this ) ;
        if ( labeling_ == 0 )
		{
			std::cout << "Label-setting algorithm" << std::endl; 
			try {
				combined_ls_algorithm( graph_, automaton_, Multimodal::Vertex( &graph_.road, request_.origin ), predecessor_pmap, potential_pmap, tt_pmap, node_penalty_pmap, vis ); 
			}
			catch ( path_found_exception& ) {// Dijkstra has been short cut
			}
		}	
		else if ( labeling_ == 1 ) {
			std::cout << "Label-correcting algorithm" << std::endl; 
			combined_lc_algorithm( graph_, automaton_, Multimodal::Vertex( &graph_.road, request_.origin ), predecessor_pmap, potential_pmap, tt_pmap, node_penalty_pmap, vis ); 
			found_ = false; 
        }
        metrics_[ "examined_edges" ] = examined_edges_;
        metrics_[ "iterations" ] = examined_vertices_; 

		if (found_) {
			Path path = reorder_path( predecessor_map, Multimodal::Vertex( &graph_.road, request_.origin ), destination_ ); 
			add_roadmap( path ); 
		}
		
        time_ += timer.elapsed(); 
		time_algo_ += timer.elapsed(); 
		metrics_[ "time_s" ] = time_; 
		metrics_[ "time_algo_s" ] = time_algo_; 
    }
    
    Path reorder_path( std::map<Object, Object>& pred_map, Multimodal::Vertex departure, Multimodal::Vertex arrival )
    {
        Path path;
        Object current;
        current.first = arrival ;
        current.second = 0 ;
        bool found = true;

        while ( current.first != departure ) {
            path.push_front( current );
            if ( pred_map[ current ].first == current.first ) {
                found = false;
                break;
            }
            current = pred_map[ current ];
        }

        if ( !found ) {
            throw std::runtime_error( "No path found" );
        }

        current.first = departure ;
        current.second = 0;
        path.push_front( current );
        return path;
    }

    void add_roadmap( const Path& path ) {
        result_.push_back( Roadmap() );
        Roadmap& roadmap = result_.back();

        roadmap.total_costs[ CostDuration ] = 0.0;

        std::list<Object>::const_iterator previous = path.begin();
        std::list<Object>::const_iterator it = ++previous;
        --previous;

        for ( ; it != path.end(); ++it ) {
            Roadmap::Step* mstep = 0;

            if ( ( previous->first.type == Multimodal::Vertex::Road ) && ( it->first.type == Multimodal::Vertex::Road ) ) {
                mstep = new Roadmap::RoadStep();
                Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( mstep );
                Road::Edge e;
                bool found = false;
                boost::tie( e, found ) = edge( previous->first.road_vertex, it->first.road_vertex, *it->first.road_graph );

                if ( !found ) throw std::runtime_error( "Can't find the road edge !" );

                step->road_edge = e;
                step->costs[ CostDuration ] = potential_map_.at( *it ) - potential_map_.at( *previous ) ;
            }

            else if ( ( previous->first.type == Multimodal::Vertex::PublicTransport ) && ( it->first.type == Multimodal::Vertex::PublicTransport ) ) {
                mstep = new Roadmap::PublicTransportStep();
                Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( mstep );
                PublicTransport::Edge e;
                bool found = false;
                boost::tie( e, found ) = edge( previous->first.pt_vertex, it->first.pt_vertex, *it->first.pt_graph );

                if ( !found )
                    throw std::runtime_error( "Can't find the pt edge !" );

                step->section = e;
                //step->trip_id = trip_map_[ target( e, pt_graph ) ];
                step->wait = 0; //wait_map_[ source( e, pt_graph ) ];

                // find the network_id
                for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports.begin(); nit != graph_.public_transports.end(); ++nit ) {
                    if ( it->first.pt_graph == &nit->second ) {
                        step->network_id = nit->first;
                        break;
                    }
                }
                step->costs[ CostDuration ] = potential_map_.at( *it ) - potential_map_.at( *previous ) - step->wait ;
            }
            else {
                // Make a multimodal edge and copy it into the roadmap as a 'generic' step
                mstep = new Roadmap::GenericStep( Multimodal::Edge( previous->first, it->first ) );
                mstep->costs[ CostDuration ] = potential_map_.at( *it ) - potential_map_.at( *previous ) ;
            }

            roadmap.steps.push_back( mstep );

            // build the multimodal edge to find corresponding costs
            // we don't use edge() since it will loop over the whole graph each time
            // we assume the edge exists in these maps
            //Multimodal::Edge me( *previous, *it );
            mstep->costs[ CostDuration ] = potential_map_.at( *it ) - potential_map_.at( *previous ) ; // - mstep->wait;
            roadmap.total_costs[ CostDuration ] += mstep->costs[ CostDuration ];

            previous = it;
        }
    }


    virtual void vertex_accessor( Multimodal::Vertex v, int access_type ) {
		if ( access_type == Plugin::ExamineAccess ) {
			if ( v == destination_ ) { 
				found_ = true; 
				throw path_found_exception();
			}     			
			examined_vertices_++;
		}
		
		// Calls road_vertex_accessor and pt_vertex_accessor
		if ( v.type == Multimodal::Vertex::Road )
			road_vertex_accessor( v.road_vertex, access_type ) ;
	}
        
        
	virtual void road_vertex_accessor( Road::Vertex v, int access_type ) {
		if ( access_type == Plugin::ExamineAccess ) {
			if ( verbose_ ) {
                            cout << "Examining road vertex " << graph_.road[ v ].db_id() << endl; 
			}
		}
	} 
    
	
    void cleanup() {}

};

Automaton<Road::Edge> HomogenTMRRoadPlugin::automaton_ = Automaton<Road::Edge>(); 
}

DECLARE_TEMPUS_PLUGIN( "homogen_tmr_road_plugin", Tempus::HomogenTMRRoadPlugin )



