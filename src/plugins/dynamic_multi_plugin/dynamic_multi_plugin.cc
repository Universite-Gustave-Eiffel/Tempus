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
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/iteration_macros.hpp>


#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"
#include "utils/graph_db_link.hh"
#include "utils/timer.hh"
#include "utils/field_property_accessor.hh"
#include "utils/function_property_accessor.hh"
#include "utils/associative_property_map_default_value.hh"


#include "cost_calculator.hh"
#include "algorithms.hh"
#include "automaton.hh"

using namespace std;

namespace Tempus {


    struct Triple {
	Multimodal::Vertex vertex;
	Automaton<Road::Edge>::Graph::vertex_descriptor state; 
	unsigned int mode; 
	
	bool operator==( const Tempus::Triple& t ) const {
            return vertex == t.vertex && state == t.state && mode == t.mode;
	}
	bool operator!=( const Tempus::Triple& t ) const {
            return vertex != t.vertex || state != t.state || mode != t.mode;
	}
        bool operator<( const Triple& other ) const
        {
            // impose an order
            if ( vertex == other.vertex ) {
                if ( state == other.state ) {
                    return mode < other.mode;
                }
                else {
                    return state < other.state;
                }
            }
            return vertex < other.vertex;
        }
    } ; 

    typedef std::list<Triple> Path; 

    typedef std::map< Triple, double > PotentialMap; 
    typedef std::map< Triple, db_id_t > TripMap; 
    typedef std::map< Triple, Triple > PredecessorMap; 

    class DynamicMultiPlugin : public Plugin {
    public:

        static const OptionDescriptionList option_descriptions() {
            OptionDescriptionList odl; 
            odl.declare_option( "timetable_frequency", "From timetables (0), frequencies (1) travel time estimation", 0);         
            odl.declare_option( "verbose_algo", "Verbose algorithm: vertices and edges traversal", false);
            odl.declare_option( "verbose", "Verbose general processing", true); 
            odl.declare_option( "min_transfer_time", "Minimum time necessary for a transfer to be done (in min)", 2); 
            odl.declare_option( "walking_speed", "Average walking speed (km/h)", 3.6); 
            odl.declare_option( "cycling_speed", "Average cycling speed (km/h)", 12); 
            odl.declare_option( "car_parking_search_time", "Car parking search time (min)", 5); 
            return odl;
        }

        DynamicMultiPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
            current_day_ = boost::gregorian::from_string("1970/1/1"); 
        }

        virtual ~DynamicMultiPlugin() {
        }
    
    protected:
        // Plugin options
        double timetable_frequency_; // travel time calculation mode
        bool verbose_algo_; // verbose vertex and edge traversal 
        bool verbose_; // Verbose processing (except algorithm)
        double min_transfer_time_; // Minimum time necessary for a transfer to be done (in minutes) 
	double walking_speed_; // Average walking speed
	double cycling_speed_; // Average cycling speed 
	double car_parking_search_time_; // Parking search time for cars
    
        // Plugin metrics
        int iterations_; // Number of iterations
        double time_; // Time elapsed for pre_process() and process() 
        double time_algo_; // Time elapsed for process()
    
        // Other attributes
        TimetableMap timetable_; // Timetable data for the current request
        FrequencyMap frequency_; // Frequency data for the current request
        Multimodal::Vertex destination_; // Current request destination 
        Date current_day_; // Day for which timetable or frequency data are loaded
        static Automaton<Road::Edge> automaton_; 
        map< Multimodal::Vertex, unsigned int > available_vehicles_; 
        static map< Multimodal::Edge, unsigned int > allowed_modes_; 
        PotentialMap potential_map_;     
        PredecessorMap pred_map_; 
        PotentialMap wait_map_; 
        TripMap trip_map_; 
	
        struct path_found_exception {};

    public: 
        static void post_build() { 
            Road::Graph& road_graph = Application::instance()->graph().road;
		
            PQImporter psql( Application::instance()->db_options() ); 
            Road::Restrictions restrictions = psql.import_turn_restrictions( road_graph );
            std::cout << "Turn restrictions imported" << std::endl;
		
            automaton_.build_graph( restrictions ) ;
            std::cout << "Automaton built" << std::endl; 
		
            std::ofstream ofs("../out/all_movements_edge_automaton.dot"); // output to graphviz
            Automaton<Road::Edge>::NodeWriter nodeWriter( automaton_.automaton_graph_ );
            Automaton<Road::Edge>::ArcWriter arcWriter( automaton_.automaton_graph_, road_graph );
            boost::write_graphviz( ofs, automaton_.automaton_graph_, nodeWriter, arcWriter);
            std::cout << "Automaton exported to graphviz" << std::endl; 
        }
    
        virtual void pre_process( Request& request ) {
    	
            // Initialize metrics
            time_=0; 
            time_algo_=0; 
            iterations_=0; 
    	
            // Initialize timer for preprocessing time
            Timer timer; 
    	
            // Check graph
            REQUIRE( graph_.public_transports.size() >= 1 );
            PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
    	
            // Check request and clear result
            REQUIRE( request.check_consistency() );
            REQUIRE( request.steps.size() == 1 );
            REQUIRE( vertex_exists( request.origin, graph_ ) );
            REQUIRE( vertex_exists( request.destination(), graph_ ) );
            request_ = request; 

            if ( request.optimizing_criteria[0] != CostDuration ) 
                throw std::invalid_argument( "Unsupported optimizing criterion" ); 
        
            if ( verbose_ ) cout << "Road origin node ID = " << graph_.road[request_.origin].db_id << ", road destination node ID = " << graph_.road[request_.destination()].db_id << endl;
            result_.clear(); 
            	
            // Get plugin options 		
            get_option( "verbose", verbose_ ); 
            get_option( "verbose_algo", verbose_algo_ ); 
            get_option( "timetable_frequency", timetable_frequency_ ); 
            get_option( "min_transfer_time", min_transfer_time_ ); 
            get_option( "walking_speed", walking_speed_ ); 
            get_option( "cycling_speed", cycling_speed_ ); 
            get_option( "car_parking_search_time", car_parking_search_time_ ); 
    	
            // If current date changed, reload timetable / frequency
            if ( current_day_ != request_.steps[0].constraint.date_time.date() )  {
        	current_day_ = request_.steps[0].constraint.date_time.date(); 
        	if (timetable_frequency_ == 0) // timetable model 
        	{
                    // Charging necessary timetable data for request processing
                    Db::Result res = db_.exec( ( boost::format( "SELECT t1.stop_id as origin_stop, t2.stop_id as destination_stop, "
                                                                "t1.trip_id, extract(epoch from t1.arrival_time)/60 as departure_time, extract(epoch from t2.departure_time)/60 as arrival_time "
                                                                "FROM tempus.pt_stop_time t1, tempus.pt_stop_time t2, tempus.pt_trip "
                                                                "WHERE t1.trip_id = t2.trip_id AND t1.stop_sequence + 1 = t2.stop_sequence AND pt_trip.id = t1.trip_id "
                                                                "AND pt_trip.service_id IN "
                                                                "("
                                                                "	(WITH calend AS ("
                                                                "		SELECT service_id, start_date, end_date, ARRAY[sunday, monday, tuesday, wednesday, thursday, friday, saturday] AS days "
                                                                "		FROM tempus.pt_calendar	)"
                                                                "	SELECT service_id "
                                                                "		FROM calend "
                                                                "		WHERE days[(SELECT EXTRACT(dow FROM TIMESTAMP '%1%')+1)] AND start_date <= '%1%' AND end_date >= '%1%' ) "
                                                                "EXCEPT ( "
                                                                "	SELECT service_id "
                                                                "		FROM tempus.pt_calendar_date "
                                                                "		WHERE calendar_date='%1%' AND exception_type=2"
                                                                "	) "
                                                                "UNION ("
                                                                "	SELECT service_id "
                                                                "		FROM tempus.pt_calendar_date	"
                                                                "		WHERE calendar_date='%1%' AND exception_type=1"
                                                                "	)"
                                                                ")") % current_day_ ).str() ); 
				
                    for ( size_t i = 0; i < res.size(); i++ ) {
                        PublicTransport::Vertex departure, arrival; 
                        PublicTransport::Edge e; 
                        bool found; 
                        boost::tie( departure, found ) = vertex_from_id( res[i][0].as<int>(), pt_graph );
                        boost::tie( arrival, found ) = vertex_from_id( res[i][1].as<int>(), pt_graph ); 
                        boost::tie( e, found ) = boost::edge( departure, arrival, pt_graph );
                        TimetableData t; 
                        t.trip_id=res[i][2].as<int>(); 
                        t.arrival_time=res[i][4].as<double>(); 
					
                        timetable_.insert( std::make_pair(e, map<double, TimetableData>() ) ); 
                        timetable_.at(e).insert( std::make_pair( res[i][3].as<double>(), t ) ) ; 				
                    }
                }
                else if (timetable_frequency_ == 1) // frequency model
                {
                    // Charging necessary frequency data for request processing 
                    Db::Result res = db_.exec( ( boost::format( "SELECT t1.stop_id as origin_stop, t2.stop_id as destination_stop, "
                                                                "t1.trip_id, extract(epoch from pt_frequency.start_time)/60 as start_time, extract(epoch from pt_frequency.end_time)/60 as end_time, "
                                                                "pt_frequency.headway_secs/60 as headway, extract(epoch from t2.departure_time - t1.arrival_time)/60 as travel_time "
                                                                "FROM tempus.pt_stop_time t1, tempus.pt_stop_time t2, tempus.pt_trip, tempus.pt_frequency "
                                                                "WHERE t1.trip_id=t2.trip_id AND t1.stop_sequence + 1 = t2.stop_sequence AND pt_trip.id=t1.trip_id AND pt_frequency.trip_id = t1.trip_id "
                                                                "AND pt_trip.service_id IN "
                                                                "("
                                                                "	(WITH calend AS ("
                                                                "		SELECT service_id, start_date, end_date, ARRAY[sunday, monday, tuesday, wednesday, thursday, friday, saturday] AS days "
                                                                "		FROM tempus.pt_calendar	)"
                                                                "	SELECT service_id "
                                                                "		FROM calend "
                                                                "		WHERE days[(SELECT EXTRACT(dow FROM TIMESTAMP '%1%')+1)] AND start_date <= '%1%' AND end_date >= '%1%' ) "
                                                                "EXCEPT ( "
                                                                "	SELECT service_id "
                                                                "		FROM tempus.pt_calendar_date "
                                                                "		WHERE calendar_date='%1%' AND exception_type=2"
                                                                "	) "
                                                                "UNION ("
                                                                "	SELECT service_id "
                                                                "		FROM tempus.pt_calendar_date	"
                                                                "		WHERE calendar_date='%1%' AND exception_type=1"
                                                                "	)"
                                                                ")") % current_day_ ).str() ); 
				
                    for ( size_t i = 0; i < res.size(); i++ ) {
                        PublicTransport::Vertex departure, arrival; 
                        PublicTransport::Edge e; 
                        bool found; 
                        boost::tie( departure,found ) = vertex_from_id( res[i][0].as<int>(), pt_graph );
                        boost::tie( arrival, found ) = vertex_from_id( res[i][1].as<int>(), pt_graph ); 
                        boost::tie( e, found ) = boost::edge( departure, arrival, pt_graph );
                        FrequencyData f; 
                        f.trip_id=res[i][2].as<int>(); 
                        f.end_time=res[i][4].as<double>(); 
                        f.headway=res[i][5].as<double>(); 
                        f.travel_time=res[i][6].as<double>(); 
					
                        frequency_.insert( std::make_pair(e, map<double, FrequencyData>() ) ); 
                        frequency_.at(e).insert( std::make_pair( res[i][3].as<double>(), f ) ) ; 	
                    }
                }
            }
        
        
            // Load vehicle positions
            for ( Multimodal::Graph::PoiList::const_iterator p = graph_.pois.begin(); p != graph_.pois.end(); p++ )
            {
        	if (p->second.poi_type == POI::TypeSharedCarPoint) 
                    available_vehicles_.at(Multimodal::Vertex(&p->second)) = 2 ; 
        	else if (p->second.poi_type == POI::TypeSharedCyclePoint)
                    available_vehicles_[Multimodal::Vertex(&p->second)] = 4; 
            }
        
                
            // Update timer for preprocessing
            time_+=timer.elapsed(); 
        }
        
        virtual void process() { 
            // Initialize timer for algo processing time
            Timer timer; 
        
            // Get origin and destination nodes
            Multimodal::Vertex origin = Multimodal::Vertex( &graph_.road, request_.origin );
            destination_ = Multimodal::Vertex( &graph_.road, request_.steps[0].destination ); 
        
            Triple origin_o;
            origin_o.vertex = origin;
            origin_o.state = automaton_.initial_state_; 
            origin_o.mode = 2; 
        
            // Initialize the potential map
            associative_property_map_default_value< PotentialMap > potential_pmap( potential_map_, std::numeric_limits<double>::max() ); // adaptation to a property map : infinite default value 
            put( potential_pmap, origin_o, request_.steps[0].constraint.date_time.time_of_day().total_seconds()/60 ) ; /// Potential of the source node is initialized to the departure time (requires dijkstra_shortest_paths_no_init to be used)
        
            // Initialize the predecessor map
            boost::associative_property_map< PredecessorMap > pred_pmap( pred_map_ );  // adaptation to a property map
            put( pred_pmap, origin_o, origin_o ); 
		
            // Initialize the trip map
            boost::associative_property_map< TripMap > trip_pmap( trip_map_ ); 
		
            // Initialize the wait map
            boost::associative_property_map< PotentialMap > wait_pmap( wait_map_ ); 
				
            // Define and initialize the cost calculator
            CostCalculator cost_calculator( timetable_, frequency_, request_.allowed_transport_types, available_vehicles_, walking_speed_, cycling_speed_, min_transfer_time_, car_parking_search_time_ ); 
                
            Tempus::PluginGraphVisitor vis ( this ) ;
            try {	
                combined_ls_algorithm_no_init( graph_, automaton_, origin_o, pred_pmap, potential_pmap, cost_calculator, trip_pmap, wait_pmap, request_.allowed_transport_types, vis );  
            }
            catch ( path_found_exception& ) {// Dijkstra has been short cut when the destination node is reached
            }
        
            metrics_[ "iterations" ] = iterations_; 
		
            Triple destination_o; 
            destination_o.vertex = destination_; 
            destination_o.state = 0; 
            destination_o.mode = 2; 
		
            Path path = reorder_path( origin_o, destination_o ); 
            add_roadmap( path ); 

            time_ += timer.elapsed(); 
            time_algo_ += timer.elapsed(); 
            metrics_[ "time_s" ] = time_; 
            metrics_[ "time_algo_s" ] = time_algo_; 
        }

        /*virtual void vertex_accessor( Triple triple, int access_type ) {
          if ( access_type == Plugin::ExamineAccess ) {
          if ( triple.vertex == destination_ ) // If destination is reached, a triple with state 0 and mode 2 is added to the potential and predecessor maps, to allow the reorder_path function to work  
          {
          if ( ( triple.state != 0 ) && ( triple.mode != 2 ) ) {
          Triple additional_triple; 
          additional_triple.vertex = triple.vertex; 
          additional_triple.state = 0; 
          additional_triple.mode = 2; 
          potential_map_[additional_triple] = potential_map_[ triple ]; 
          pred_map_[additional_triple] = pred_map_[ triple ]; 
          } 
          throw path_found_exception(); // For a label-setting algorithm, the search is interrupted when the destination node is examined
          }
    		
          iterations_++;
          if ( verbose_algo_ ) cout << " Potential value : "<<potential_map_[ triple ]<< endl; 
          }
        
          // Calls road_vertex_accessor and pt_vertex_accessor
          if ( triple.vertex.type == Multimodal::Vertex::Road ) {
          if ( verbose_algo_ ) cout << "Examining triple : (road vertex = " << graph_.road[ triple.vertex.road_vertex ].db_id << ", state = " << triple.state << ", mode = " << triple.mode << ")" << endl; 
          }
          else if ( triple.vertex.type == Multimodal::Vertex::PublicTransport ) { 
          if ( verbose_algo_ ) cout << "Examining triple : (pt vertex = " << graph_.public_transports.begin()->second[ triple.vertex.pt_vertex ].db_id << ", state = " << triple.state << ", mode = " << triple.mode << ")" << endl; 
          }
          }
        
          virtual void edge_accessor( Multimodal::Edge e, int access_type ) {
          if ( e.connection_type() == Multimodal::Edge::Transport2Transport ) {
          PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
          PublicTransport::Edge pt_e ;
          bool found; 
          boost::tie( pt_e, found ) = edge( source(e, graph_).pt_vertex, target(e, graph_).pt_vertex, pt_graph );

          if ( access_type == Plugin::ExamineAccess ) {
          if ( verbose_algo_ ) {
          cout << "	Examining edge (" << pt_graph[ source(pt_e, pt_graph) ].db_id << ", " << pt_graph[ target(pt_e, pt_graph) ].db_id << ")" << endl;
          }
          }
          }
          } */
    
        Path reorder_path( Triple departure, Triple arrival )
        {
            Path path; 
            Triple current = arrival;
            bool found = true; 

            while ( current.vertex != departure.vertex ) {
                path.push_front( current );

                if ( pred_map_[ current ] == current ) {
                    found = false;
                    break;
                }

                current = pred_map_[ current ]; 
            }

            if ( !found ) {
                throw std::runtime_error( "No path found" );
            }

            path.push_front( departure ); 
            return path; 
	}
    
        void add_roadmap( const Path& path ) {
            result_.push_back( Roadmap() ); 
            Roadmap& roadmap = result_.back();

            roadmap.total_costs[ CostDuration ] = 0.0; 
		
            std::list< Triple >::const_iterator previous = path.begin();
            std::list< Triple >::const_iterator it = ++previous;
            --previous; 

            for ( ; it != path.end(); ++it ) {
                Roadmap::Step* mstep = 0;

                if ( previous->vertex.type == Multimodal::Vertex::Road && it->vertex.type == Multimodal::Vertex::Road ) {
                    mstep = new Roadmap::RoadStep();
                    Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( mstep );
                    Road::Edge e;
                    bool found = false;
                    boost::tie( e, found ) = edge( previous->vertex.road_vertex, it->vertex.road_vertex, *it->vertex.road_graph ); 

                    if ( !found ) {
                        throw std::runtime_error( "Can't find the road edge !" );
                    }

                    step->road_edge = e;
                    step->transport_type = it->mode ;   
                    step->costs[ CostDuration ] = potential_map_[ *it ] - potential_map_[ *previous ]  ; 
                }

                else if ( previous->vertex.type == Multimodal::Vertex::PublicTransport && it->vertex.type == Multimodal::Vertex::PublicTransport ) {
                    mstep = new Roadmap::PublicTransportStep();
                    Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( mstep );
                    PublicTransport::Edge e;
                    bool found = false;
                    boost::tie( e, found ) = edge( previous->vertex.pt_vertex, it->vertex.pt_vertex, *(it->vertex).pt_graph ); 

                    if ( !found ) 
                        throw std::runtime_error( "Can't find the pt edge !" ); 
				
                    step->section = e; 
                    step->trip_id = trip_map_[ *it ]; 
                    step->wait = wait_map_[ *previous ]; 
				
                    // find the network_id
                    for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports.begin(); nit != graph_.public_transports.end(); ++nit ) {
                        if ( it->vertex.pt_graph == &nit->second ) {
                            step->network_id = nit->first;
                            break;
                        }
                    }
                    step->costs[ CostDuration ] = potential_map_[ *it ] - potential_map_[ *previous ] - step->wait;
                }
                else {
                    // Make a multimodal edge and copy it into the roadmap as a 'generic' step
                    mstep = new Roadmap::GenericStep( Multimodal::Edge( previous->vertex, it->vertex ) );
                    mstep->costs[ CostDuration ] = potential_map_[ *it ] - potential_map_[ *previous ]; 
                }

                roadmap.steps.push_back( mstep );

                // build the multimodal edge to find corresponding costs
                // we don't use edge() since it will loop over the whole graph each time
                // we assume the edge exists in these maps
                //Multimodal::Edge me( *previous, *it );
                //mstep->costs[ CostDuration ] = potential_map[ vertex_index[*it] ] - potential_map[ vertex_index[*previous] ] - mstep->wait;
                roadmap.total_costs[ CostDuration ] += mstep->costs[ CostDuration ];

                previous = it; 
            }
	}


        void cleanup() {
            // nothing to clean up
        }
    
    }; 

    Automaton<Road::Edge> DynamicMultiPlugin::automaton_;
    map< Multimodal::Edge, unsigned int > DynamicMultiPlugin::allowed_modes_;
}
DECLARE_TEMPUS_PLUGIN( "dynamic_multi_plugin", Tempus::DynamicMultiPlugin )




