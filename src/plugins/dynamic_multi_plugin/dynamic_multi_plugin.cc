/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "dynamic_multi_plugin.hh"
#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>

#include "pgsql_importer.hh"
#include "utils/timer.hh"
#include "utils/field_property_accessor.hh"
#include "utils/function_property_accessor.hh"
#include "utils/associative_property_map_default_value.hh"

#include "automaton_lib/cost_calculator.hh"
#include "automaton_lib/automaton.hh"
#include "algorithms.hh"

using namespace std;

namespace Tempus {

// special visitor to shortcut dijkstra
class DestinationDetectorVisitor
{
public:
    DestinationDetectorVisitor( const Multimodal::Graph& graph, const Road::Vertex& destination, bool pvad = true, bool verbose = false ) :
        graph_(graph),
        destination_(destination),
        pvad_(pvad),
        verbose_(verbose) {}

    // the exception used to shortcut dijkstra
    struct path_found_exception
    {
        path_found_exception( const Triple& t ) : destination(t) {}
        Triple destination;
    };

    void examine_vertex( const Triple& t, const Multimodal::Graph& )
    {
        if ( t.vertex.type() == Multimodal::Vertex::Road && t.vertex.road_vertex() == destination_ ) {
            TransportMode mode = graph_.transport_mode( t.mode ).get();
            if ( !mode.must_be_returned() && !mode.is_public_transport() ) {
                if ( (t.mode == TransportModePrivateCar) || (t.mode == TransportModePrivateBicycle) ) {
                    if ( pvad_ ) {
                        throw path_found_exception( t );
                    }
                }
                else {
                    throw path_found_exception( t );
                }
            }
        }
        if (verbose_) {
            std::cout << "Examine triple (vertex=" << t.vertex << ", mode=" << t.mode << ", state=" << t.state << ")" << std::endl;
        }
    }

    void finish_vertex( const Triple&, const Multimodal::Graph& )
    {
    }

    void examine_edge( const Multimodal::Edge& e, const Multimodal::Graph& )
    {
        if (verbose_) {
            std::cout << "Examine edge " << e << std::endl;
        }
    }

    void edge_relaxed( const Multimodal::Edge&, unsigned int, const Multimodal::Graph& )
    {
    }

private:
    const Multimodal::Graph& graph_;
    Road::Vertex destination_;
    // private vehicule at destination
    bool pvad_;
    bool verbose_;
};

const DynamicMultiPlugin::OptionDescriptionList DynamicMultiPlugin::option_descriptions()
{
    Plugin::OptionDescriptionList odl; 
    odl.declare_option( "timetable_frequency", "From timetables (0), frequencies (1) travel time estimation", 0);         
    odl.declare_option( "verbose_algo", "Verbose algorithm: vertices and edges traversal", false);
    odl.declare_option( "verbose", "Verbose general processing", true); 
    odl.declare_option( "min_transfer_time", "Minimum time necessary for a transfer to be done (in min)", 2); 
    odl.declare_option( "walking_speed", "Average walking speed (km/h)", 3.6); 
    odl.declare_option( "cycling_speed", "Average cycling speed (km/h)", 12); 
    odl.declare_option( "car_parking_search_time", "Car parking search time (min)", 5); 
    return odl;
}

const DynamicMultiPlugin::PluginParameters DynamicMultiPlugin::plugin_parameters()
{
    Plugin::PluginParameters params; 
    params.supported_optimization_criteria.push_back( CostDuration );
    return params;
}

void DynamicMultiPlugin::post_build()
{
    const Road::Graph& road_graph = Application::instance()->graph()->road();
		
    PQImporter psql( Application::instance()->db_options() ); 
    Road::Restrictions restrictions = psql.import_turn_restrictions( road_graph, Application::instance()->schema_name() );
    std::cout << "Turn restrictions imported" << std::endl;
		
    s_.automaton.build_graph( restrictions ) ;
    std::cout << "Automaton built" << std::endl; 
		
    std::ofstream ofs("all_movements_edge_automaton.dot"); // output to graphviz
    Automaton<Road::Edge>::NodeWriter nodeWriter( s_.automaton.automaton_graph_ );
    Automaton<Road::Edge>::ArcWriter arcWriter( s_.automaton.automaton_graph_, road_graph );
    boost::write_graphviz( ofs, s_.automaton.automaton_graph_, nodeWriter, arcWriter);
    std::cout << "Automaton exported to graphviz" << std::endl; 
}
    
void DynamicMultiPlugin::pre_process( Request& request )
{
    // Initialize metrics
    time_=0; 
    time_algo_=0; 
    iterations_=0; 
    	
    // Initialize timer for preprocessing time
    Timer timer; 
    	
    // Check request and clear result
    REQUIRE( request.allowed_modes().size() >= 1 );
    REQUIRE( vertex_exists( request.origin(), graph_.road() ) );
    REQUIRE( vertex_exists( request.destination(), graph_.road() ) );
    request_ = request; 

    if ( request.optimizing_criteria()[0] != CostDuration ) 
        throw std::invalid_argument( "Unsupported optimizing criterion" ); 
        
    if ( verbose_ ) cout << "Road origin node ID = " << graph_.road()[request_.origin()].db_id() << ", road destination node ID = " << graph_.road()[request_.destination()].db_id() << endl;
    result_.clear(); 
            	
    // Get plugin options 		
    get_option( "verbose", verbose_ ); 
    get_option( "verbose_algo", verbose_algo_ ); 
    get_option( "timetable_frequency", timetable_frequency_ ); 
    get_option( "min_transfer_time", min_transfer_time_ ); 
    get_option( "walking_speed", walking_speed_ ); 
    get_option( "cycling_speed", cycling_speed_ ); 
    get_option( "car_parking_search_time", car_parking_search_time_ );

    // look for public transports in allowed modes
    bool pt_allowed = false;
    // look also for private modes
    bool private_mode = false;
    for ( size_t i = 0; i < request.allowed_modes().size(); i++ ) {
        db_id_t mode_id = request.allowed_modes()[i];
        if (graph_.transport_mode( mode_id )->is_public_transport() ) {
            pt_allowed = true;
        }
        else if ( (mode_id == TransportModePrivateBicycle) || (mode_id == TransportModePrivateCar) ) {
            private_mode = true;
        }
    }
    // resolve private parking location
    if ( private_mode ) {
        if ( request_.parking_location() ) {
            parking_location_ = request_.parking_location().get();
        }
        else {
            // place the private parking at the origin
            parking_location_ = request_.origin();
        }
    }
    	
    // If current date changed, reload timetable / frequency
    if ( pt_allowed && (graph_.public_transports().size() > 0) && (s_.current_day != request_.steps()[0].constraint().date_time().date()) )  {
        const PublicTransport::Graph& pt_graph = *graph_.public_transports().begin()->second;
        std::cout << "load timetable" << std::endl;
        s_.current_day = request_.steps()[0].constraint().date_time().date();

        // cache graph id to descriptor
        // FIXME - integrate the cache into the graphs ?
        std::map<db_id_t, PublicTransport::Vertex> vertex_from_id_;
        PublicTransport::Graph::vertex_iterator vit, vend;
        for ( boost::tie( vit, vend ) = vertices( pt_graph ); vit != vend; ++vit ) {
            vertex_from_id_[ pt_graph[*vit].db_id() ] = *vit;
        }

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
                                                        ")") % boost::gregorian::to_simple_string(s_.current_day) ).str() ); 
				
            for ( size_t i = 0; i < res.size(); i++ ) {
                PublicTransport::Vertex departure, arrival; 
                PublicTransport::Edge e; 
                bool found;
                departure = vertex_from_id_[ res[i][0].as<db_id_t>() ];
                arrival = vertex_from_id_[ res[i][1].as<db_id_t>() ]; 
                boost::tie( e, found ) = boost::edge( departure, arrival, pt_graph );
                TimetableData t; 
                t.trip_id=res[i][2].as<int>(); 
                t.arrival_time=res[i][4].as<double>(); 
                s_.timetable.insert( std::make_pair(e, map<double, TimetableData>() ) ); 
                s_.timetable[e].insert( std::make_pair( res[i][3].as<double>(), t ) ) ; 				
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
                                                        ")") % boost::gregorian::to_simple_string(s_.current_day) ).str() ); 
				
            for ( size_t i = 0; i < res.size(); i++ ) {
                PublicTransport::Vertex departure, arrival; 
                PublicTransport::Edge e; 
                bool found; 
                departure = vertex_from_id_[ res[i][0].as<db_id_t>() ];
                arrival = vertex_from_id_[ res[i][1].as<db_id_t>() ];
                boost::tie( e, found ) = boost::edge( departure, arrival, pt_graph );
                FrequencyData f; 
                f.trip_id=res[i][2].as<int>(); 
                f.end_time=res[i][4].as<double>(); 
                f.headway=res[i][5].as<double>(); 
                f.travel_time=res[i][6].as<double>(); 
					
                s_.frequency.insert( std::make_pair(e, map<double, FrequencyData>() ) ); 
                s_.frequency[e].insert( std::make_pair( res[i][3].as<double>(), f ) ) ; 	
            }
        }
    }
        
#if 0
    // Load vehicle positions
    for ( Multimodal::Graph::PoiList::const_iterator p = graph_.pois.begin(); p != graph_.pois.end(); p++ )
    {
        if (p->second.poi_type() == POI::TypeSharedCarPoint)
            available_vehicles_[Multimodal::Vertex(&p->second)] = 2;
        else if (p->second.poi_type() == POI::TypeSharedCyclePoint)
            available_vehicles_[Multimodal::Vertex(&p->second)] = 4;
    }
#endif
                
    // Update timer for preprocessing
    time_+=timer.elapsed();
}
        
void DynamicMultiPlugin::process()
{
    // Initialize timer for algo processing time
    Timer timer;

    // Get origin and destination nodes
    Multimodal::Vertex origin = Multimodal::Vertex( &graph_.road(), request_.origin() );
    destination_ = Multimodal::Vertex( &graph_.road(), request_.destination() );

    Triple origin_o;
    origin_o.vertex = origin;
    origin_o.state = s_.automaton.initial_state_;
    // take the first mode allowed
    origin_o.mode = request_.allowed_modes()[0];

    // Initialize the potential map
    // adaptation to a property map : infinite default value
    associative_property_map_default_value< PotentialMap > potential_pmap( potential_map_, std::numeric_limits<double>::max() );
    double ts = request_.steps()[0].constraint().date_time().time_of_day().total_seconds()/60;
    /// Potential of the source node is initialized to the departure time (requires dijkstra_shortest_paths_no_init to be used)
    put( potential_pmap, origin_o, ts ) ;
        
    // Initialize the predecessor map
    boost::associative_property_map< PredecessorMap > pred_pmap( pred_map_ );  // adaptation to a property map
    put( pred_pmap, origin_o, origin_o );

    // Initialize the trip map
    boost::associative_property_map< TripMap > trip_pmap( trip_map_ );

    // Initialize the wait map
    boost::associative_property_map< PotentialMap > wait_pmap( wait_map_ );

    // Define and initialize the cost calculator
    CostCalculator cost_calculator( s_.timetable, s_.frequency, request_.allowed_modes(), available_vehicles_, walking_speed_, cycling_speed_, min_transfer_time_, car_parking_search_time_, parking_location_ );

    // we cannot use the regular visitor here, since we examine tuples instead of vertices
    DestinationDetectorVisitor vis( graph_, request_.destination(), request_.steps().back().private_vehicule_at_destination(), verbose_algo_ );

    Triple destination_o;
    bool path_found = false;
    try {
        combined_ls_algorithm_no_init( graph_, s_.automaton, origin_o, pred_pmap, potential_pmap, cost_calculator, trip_pmap, wait_pmap, request_.allowed_modes(), vis );
    }
    catch ( DestinationDetectorVisitor::path_found_exception& e ) {
        // Dijkstra has been short cut when the destination node is reached
        destination_o = e.destination;
        path_found = true;
    }
        
    metrics_[ "iterations" ] = iterations_;
		
    time_ += timer.elapsed();
    time_algo_ += timer.elapsed();
    metrics_[ "time_s" ] = time_;
    metrics_[ "time_algo_s" ] = time_algo_;

    if (!path_found) {
        throw std::runtime_error( "No path found" );                
    }

    Path path = reorder_path( origin_o, destination_o );
    add_roadmap( path );
}
    
Path DynamicMultiPlugin::reorder_path( Triple departure, Triple arrival )
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
    
void DynamicMultiPlugin::add_roadmap( const Path& path )
{
    result_.push_back( Roadmap() ); 
    Roadmap& roadmap = result_.back();

    std::list< Triple >::const_iterator it = path.begin();
    std::list< Triple >::const_iterator next = it;
    next++;

    for ( ; next != path.end(); ++next, ++it ) {
        std::auto_ptr<Roadmap::Step> mstep;

        if ( it->mode == next->mode && it->vertex.type() == Multimodal::Vertex::Road && next->vertex.type() == Multimodal::Vertex::Road ) {
            mstep.reset( new Roadmap::RoadStep() );
            Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( mstep.get() );
            Road::Edge e;
            bool found = false;
            boost::tie( e, found ) = edge( it->vertex.road_vertex(), next->vertex.road_vertex(), *it->vertex.road_graph() ); 

            if ( !found ) {
                throw std::runtime_error( "Can't find the road edge !" );
            }

            step->set_road_edge( e );
            step->set_transport_mode( it->mode );
            step->set_cost( CostDuration, potential_map_[ *next ] - potential_map_[ *it ] );
        }

        else if ( it->vertex.type() == Multimodal::Vertex::PublicTransport && next->vertex.type() == Multimodal::Vertex::PublicTransport ) {
            mstep.reset( new Roadmap::PublicTransportStep() );
            Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( mstep.get() );
				
            step->set_transport_mode( it->mode );
            step->set_departure_stop( it->vertex.pt_vertex() );
            step->set_arrival_stop( next->vertex.pt_vertex() );
            step->set_trip_id(trip_map_[ *next ]);
            step->set_wait(wait_map_[ *it ]);
            step->set_departure_time( potential_map_[*it] );
            step->set_arrival_time( potential_map_[*next] );
				
            // find the network_id
            for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports().begin(); nit != graph_.public_transports().end(); ++nit ) {
                if ( it->vertex.pt_graph() == nit->second ) {
                    step->set_network_id(nit->first);
                    break;
                }
            }
            step->set_cost( CostDuration, step->arrival_time() - step->departure_time() );
        }
        else {
            // Make a multimodal edge and copy it into the roadmap as a 'generic' step
            mstep.reset( new Roadmap::TransferStep( Multimodal::Edge( it->vertex, next->vertex ) ) );
            Roadmap::TransferStep* step = static_cast<Roadmap::TransferStep*>(mstep.get());
            step->set_transport_mode( it->mode );
            step->set_final_mode( next->mode );
            step->set_cost( CostDuration, potential_map_[ *next ] - potential_map_[ *it ] );
        }

        roadmap.add_step( mstep );
    }
}

StaticVariables DynamicMultiPlugin::s_;
} 

DECLARE_TEMPUS_PLUGIN( "dynamic_multi_plugin", Tempus::DynamicMultiPlugin )




