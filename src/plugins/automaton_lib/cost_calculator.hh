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

#ifndef AUTOMATION_LIB_COST_CALCULATOR_HH
#define AUTOMATION_LIB_COST_CALCULATOR_HH

#include "reverse_multimodal_graph.hh"

namespace Tempus {

// Pedestrian speed in m/sec
#define DEFAULT_WALKING_SPEED 1.0
// Cycle speed in m/sec
#define DEFAULT_CYCLING_SPEED 5.0

// Time penalty to add when walking in and out from/to a public transport station
#define PT_STATION_PENALTY 0.1
#define POI_STATION_PENALTY 0.1

template <class RoadGraph>
double road_travel_time( const RoadGraph& road_graph, const Road::Edge& road_e, double length, const TransportMode& mode,
                         double walking_speed = DEFAULT_WALKING_SPEED, double cycling_speed = DEFAULT_CYCLING_SPEED )
{
    if ( (road_graph[ road_e ].traffic_rules() & mode.traffic_rules()) == 0 ) { // Not allowed mode 
        return std::numeric_limits<double>::infinity() ;
    }
    else if ( mode.speed_rule() == SpeedRuleCar ) {
        // FIXME, we take the speed limit for now,
        // but should take the speed profile
        return length / (road_graph[ road_e ].car_speed_limit() * 1000) * 60 ;
    }
    else if ( mode.speed_rule() == SpeedRulePedestrian ) { // Walking
        // FIXME the speed should be carried by the transport mode
        return length / (walking_speed * 1000) * 60 ;
    }
    else if ( mode.speed_rule() == SpeedRuleBicycle ) { // Bicycle
        return length / (cycling_speed * 1000) * 60 ;
    }
    else {
        return std::numeric_limits<double>::max() ;
    }
}

// Turning movement penalty function
template <typename AutomatonGraph>
double penalty ( const AutomatonGraph& graph, const typename boost::graph_traits< AutomatonGraph >::vertex_descriptor& v, unsigned traffic_rules )
{
    for (std::map<int, double >::const_iterator penaltyIt = graph[v].penalty_per_mode.begin() ; penaltyIt != graph[v].penalty_per_mode.end() ; penaltyIt++ )
    {
        // if the mode has a traffic rule in common with the penalty key
        if (traffic_rules & penaltyIt->first) return penaltyIt->second; 
    }
    return 0; 
}		

struct TimetableData {
    unsigned int trip_id;
    double arrival_time; 
};
	
struct FrequencyData {
    unsigned int trip_id; 
    double end_time; 
    double headway;
    double travel_time; 
};
	
// Edge -> transport_mode -> departure_time -> Timetable
typedef std::map<PublicTransport::Edge, std::map<int, std::map<double, TimetableData> > > TimetableMap; 
typedef std::map<PublicTransport::Edge, std::map<int, std::map<double, FrequencyData> > > FrequencyMap; 
	
class CostCalculator {
public: 
    // Constructor 
    CostCalculator( const TimetableMap& timetable, const TimetableMap& rtimetable, const FrequencyMap& frequency, const FrequencyMap& rfrequency,
                    const std::vector<db_id_t>& allowed_transport_modes, std::map<Multimodal::Vertex, db_id_t>& vehicle_nodes, 
                    double walking_speed, double cycling_speed, 
                    double min_transfer_time, double car_parking_search_time, boost::optional<Road::Vertex> private_parking ) : 
        timetable_( timetable ), rtimetable_( rtimetable ), frequency_( frequency ), rfrequency_(rfrequency),
        allowed_transport_modes_( allowed_transport_modes ), vehicle_nodes_( vehicle_nodes ), 
        walking_speed_( walking_speed ), cycling_speed_( cycling_speed ), 
        min_transfer_time_( min_transfer_time ), car_parking_search_time_( car_parking_search_time ),
        private_parking_( private_parking ) { }; 
		
    // Multimodal travel time function
    template <class Graph>
    double travel_time( const Graph& graph, const Multimodal::Edge& e, db_id_t mode_id, double initial_time, double initial_shift_time, double& final_shift_time, db_id_t initial_trip_id, db_id_t& final_trip_id, double& wait_time ) const
    {
        // default (for non-PT edges)
        final_trip_id = 0;

        if ( is_graph_reversed<Graph>::value ) {
            initial_time = - initial_time;
        }
        final_shift_time = initial_shift_time;
        wait_time = 0.0;

        const TransportMode& mode = graph.transport_modes().find( mode_id )->second;
        if ( std::find(allowed_transport_modes_.begin(), allowed_transport_modes_.end(), mode_id) != allowed_transport_modes_.end() ) 
        {
            switch ( e.connection_type() ) {  
            case Multimodal::Edge::Road2Road: {
                double c = road_travel_time( graph.road(), e.road_edge(), graph.road()[ e.road_edge() ].length(), mode,
                                         walking_speed_, cycling_speed_ ); 
                return c;
            }
                break;
		
            case Multimodal::Edge::Road2Transport: {
                if ( is_graph_reversed<Graph>::value ) {
                    if ( initial_trip_id != 0 ) {
                        // we are "coming" from a Transport2Transport
                        final_shift_time += min_transfer_time_;
                    }
                }

                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( e.target().pt_graph() );
                double abscissa = pt_graph[ e.target().pt_vertex() ].abscissa_road_section();
                Road::Edge road_e = pt_graph[ e.target().pt_vertex() ].road_edge();
						
                // if we are coming from the start point of the road
                if ( source( road_e, graph.road() ) == e.source().road_vertex() ) {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * abscissa, mode,
                                             walking_speed_, cycling_speed_ ) + PT_STATION_PENALTY;
                }
                // otherwise, that is the opposite direction
                else {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * (1 - abscissa), mode,
                                             walking_speed_, cycling_speed_ ) + PT_STATION_PENALTY;
                }
            }
                break; 
					
            case Multimodal::Edge::Transport2Road: {
                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( e.source().pt_graph() );
                double abscissa = pt_graph[ e.source().pt_vertex() ].abscissa_road_section();
                Road::Edge road_e = pt_graph[ e.source().pt_vertex() ].road_edge();
						
                // if we are coming from the start point of the road
                if ( source( road_e, graph.road() ) == e.source().road_vertex() ) {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * abscissa, mode,
                                             walking_speed_, cycling_speed_ ) + PT_STATION_PENALTY;
                }
                // otherwise, that is the opposite direction
                else {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * (1 - abscissa), mode,
                                             walking_speed_, cycling_speed_ ) + PT_STATION_PENALTY;
                }
            } 
                break;
					
            case Multimodal::Edge::Transport2Transport: { 
                PublicTransport::Edge pt_e;
                bool found = false;
                boost::tie( pt_e, found ) = public_transport_edge( e );
                BOOST_ASSERT(found);
						
                // Timetable travel time calculation
                TimetableMap::const_iterator pt_e_it = timetable_.find( pt_e );
                if ( pt_e_it != timetable_.end() ) {
                    if ( ! is_graph_reversed<Graph>::value ) {
                        const std::map<int, std::map<double,TimetableData> >& tt = pt_e_it->second;
                        // look for timetable of the given mode
                        std::map<int, std::map<double, TimetableData> >::const_iterator mit = tt.find( mode_id );
                        if ( mit == tt.end() ) { // no timetable for this mode
                            return std::numeric_limits<double>::max(); 
                        }

                        // get the time, just after initial_time
                        std::map<double, TimetableData >::const_iterator it = mit->second.lower_bound( initial_time ) ;
                        if ( it == mit->second.end() ) { // no service after this time
                            return std::numeric_limits<double>::max(); 
                        }

                        // Continue on the same trip
                        if ( it->second.trip_id == initial_trip_id ) { 
                            final_trip_id = it->second.trip_id; 
                            wait_time = 0;
                            return it->second.arrival_time - initial_time ; 
                        } 
                        // Else, no connection without transfer found, or first step
                        // Look for a service after transfer_time
                        it = mit->second.lower_bound( initial_time + min_transfer_time_ ); 
                        if ( it != mit->second.end() ) {
                            final_trip_id = it->second.trip_id;
                            wait_time = it->first - initial_time;
                            return it->second.arrival_time - initial_time;
                        }
                    }
                    else {
                        double rinitial_time = initial_time - initial_shift_time;

                        // reversed
                        const std::map<int, std::map<double,TimetableData> >& tt = rtimetable_.find(pt_e)->second;
                        // look for timetable of the given mode
                        std::map<int, std::map<double, TimetableData> >::const_iterator mit = tt.find( mode_id );
                        if ( mit == tt.end() ) { // no timetable for this mode
                            return std::numeric_limits<double>::max(); 
                        }
                        // get the time, just before initial_time (upper_bound - 1)
                        std::map< double, TimetableData >::const_iterator it = mit->second.upper_bound( rinitial_time ) ;
                        if ( it == mit->second.begin() ) { // nothing before this time
                            return std::numeric_limits<double>::max(); 
                        }
                        it--;

                        final_trip_id = it->second.trip_id; 
                        wait_time = rinitial_time - it->first;
                        final_shift_time += wait_time;
                        return it->first - it->second.arrival_time;
                    }
                }
                else if (frequency_.find( pt_e ) != frequency_.end() ) {
                    if ( ! is_graph_reversed<Graph>::value ) {
                        const std::map<int, std::map<double,FrequencyData> >& tt = frequency_.find( pt_e )->second;
                        std::map<int, std::map<double, FrequencyData> >::const_iterator mit = tt.find( mode_id );
                        if ( mit == tt.end() ) { // no timetable for this mode
                            return std::numeric_limits<double>::max(); 
                        }
                        std::map<double, FrequencyData>::const_iterator it = mit->second.upper_bound( initial_time );
                        if (it == mit->second.begin() ) { // nothing before this time
                            return std::numeric_limits<double>::max();
                        }
                        it--;
                        if ( it->second.end_time < initial_time ) {
                            // frequency-based trips are supposed not to overlap
                            // so it means this trip is not in service anymore at initial_time
                            return std::numeric_limits<double>::max();                        
                        }
                        if ( it->second.trip_id == initial_trip_id ) {
                            final_trip_id = it->second.trip_id; 
                            wait_time = 0;
                            return it->second.travel_time;
                        } 
                        // Else, no connection without transfer found
                        it = mit->second.upper_bound( initial_time + min_transfer_time_ ); 
                        if ( it != mit->second.begin() ) { 
                            it--; 
                            if ( it->second.end_time >= initial_time + min_transfer_time_ ) {
                                final_trip_id = it->second.trip_id ; 
                                wait_time = it->second.headway/2 ; 
                                return it->second.travel_time + wait_time;
                            }
                        }
                    }
                    else {
                        // reverse
                        const std::map<int, std::map<double,FrequencyData> >& tt = rfrequency_.find( pt_e )->second;
                        std::map<int, std::map<double, FrequencyData> >::const_iterator mit = tt.find( mode_id );
                        if ( mit == tt.end() ) { // no timetable for this mode
                            return std::numeric_limits<double>::max(); 
                        }
                        std::map<double, FrequencyData>::const_iterator it = mit->second.upper_bound( initial_time );
                        if (it == mit->second.end() ) { // nothing before this time
                            return std::numeric_limits<double>::max();
                        }
                        if ( it->second.end_time >= initial_time ) {
                            // frequency-based trips are supposed not to overlap
                            // so it means this trip is not in service anymore at initial_time
                            return std::numeric_limits<double>::max();                        
                        }
                        if ( it->second.trip_id == initial_trip_id ) {
                            final_trip_id = it->second.trip_id; 
                            wait_time = 0;
                            return it->second.travel_time;
                        } 
                        // Else, no connection without transfer found
                        it = mit->second.upper_bound( initial_time - min_transfer_time_ ); 
                        if ( it != mit->second.end() ) { 
                            if ( it->second.end_time < initial_time - min_transfer_time_ ) {
                                final_trip_id = it->second.trip_id;
                                wait_time = it->second.headway/2;
                                return it->second.travel_time + wait_time;
                            }
                        }
                    }
                }
            }
                break; 
					
            case Multimodal::Edge::Road2Poi: {
                Road::Edge road_e = e.target().poi()->road_edge();
                double abscissa = e.target().poi()->abscissa_road_section();

                // if we are coming from the start point of the road
                if ( source( road_e, graph.road() ) == e.source().road_vertex() ) {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * abscissa, mode,
                                             walking_speed_, cycling_speed_ ) + POI_STATION_PENALTY;
                }
                // otherwise, that is the opposite direction
                else {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * (1 - abscissa), mode,
                                             walking_speed_, cycling_speed_ ) + POI_STATION_PENALTY;
                }
            }
                break;
            case Multimodal::Edge::Poi2Road: {
                Road::Edge road_e = e.source().poi()->road_edge();
                double abscissa = e.source().poi()->abscissa_road_section();

                // if we are coming from the start point of the road
                if ( source( road_e, graph.road() ) == e.source().road_vertex() ) {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * abscissa, mode,
                                             walking_speed_, cycling_speed_ ) + POI_STATION_PENALTY;
                }
                // otherwise, that is the opposite direction
                else {
                    return road_travel_time( graph.road(), road_e, graph.road()[ road_e ].length() * (1 - abscissa), mode,
                                             walking_speed_, cycling_speed_ ) + POI_STATION_PENALTY;
                }
            }
                break;
            default: {
						
            }
            }
        }
        return std::numeric_limits<double>::max(); 
    }
		
    // Mode transfer time function : returns numeric_limits<double>::max() when the mode transfer is impossible
    template <class Graph>
    double transfer_time( const Graph& graph, const Multimodal::Edge& edge, db_id_t initial_mode_id, db_id_t final_mode_id ) const
    {
        double transf_t = 0;
        if (initial_mode_id == final_mode_id ) {
            return 0.0;
        }

        const Multimodal::Vertex& src = edge.source();
        const Multimodal::Vertex& tgt = edge.target();

        const TransportMode& initial_mode = graph.transport_modes().find( initial_mode_id )->second;
        const TransportMode& final_mode = graph.transport_modes().find( final_mode_id )->second;

        if ( initial_mode.is_public_transport() && final_mode.is_public_transport() ) {
            return 0.0;
        }

        // park shared vehicle
        if ( ( transf_t < std::numeric_limits<double>::max() ) && initial_mode.must_be_returned() ) {
            if (( tgt.type() == Multimodal::Vertex::Poi ) && ( tgt.poi()->has_parking_transport_mode( initial_mode.db_id() ) )) {
                // FIXME replace 1 by time to park a shared vehicle
                transf_t += 1;
            }
            else {
                transf_t = std::numeric_limits<double>::max();
            }                
        }
        // Parking search time for initial mode
        else if ( ( transf_t < std::numeric_limits<double>::max() ) && initial_mode.need_parking() ) {
            if ( (tgt.type() == Multimodal::Vertex::Poi ) && ( tgt.poi()->has_parking_transport_mode( initial_mode.db_id() ) ) ) {
                // FIXME more complex than that
                if (initial_mode.traffic_rules() & TrafficRuleCar ) 
                    transf_t += car_parking_search_time_ ; // Personal car
                // For bicycle, parking search time = 0
            }
            // park on the private parking
            else if ( private_parking_ && !initial_mode.is_shared() && tgt.road_vertex() == private_parking_.get() ) {
                transf_t += 1;
            }
            // park on streets
            else if ( ( tgt.type() == Multimodal::Vertex::Road ) && (src.type() == Multimodal::Vertex::Road) &&
                      ( (graph.road()[ edge.road_edge() ].parking_traffic_rules() & initial_mode.traffic_rules()) > 0 ) ) {
                if (initial_mode.traffic_rules() & TrafficRuleCar ) 
                    transf_t += car_parking_search_time_ ; // Personal car
                // For bicycle, parking search time = 0
            }
            else {
                transf_t = std::numeric_limits<double>::max();
            }
        }


        // take a shared vehicle from a POI
        if ( ( transf_t < std::numeric_limits<double>::max() ) && final_mode.is_shared() ) {
            if (( src.type() == Multimodal::Vertex::Poi ) && ( src.poi()->has_parking_transport_mode( final_mode.db_id() ) )) {
                // FIXME replace 1 by time to take a shared vehicle
                transf_t += 1;
            }
            else {
                transf_t = std::numeric_limits<double>::max();
            }
        }
        // Taking vehicle time for final mode 
        else if ( ( transf_t < std::numeric_limits<double>::max() ) && ( final_mode.need_parking() ) ) {
            if (( src.type() == Multimodal::Vertex::Poi ) && final_mode.is_shared() && ( src.poi()->has_parking_transport_mode( final_mode.db_id() ) )) {
                // shared vehicles parked on a POI
                transf_t += 1;
            }
            else if ( private_parking_ && !final_mode.is_shared() && src.road_vertex() == private_parking_.get() ) {
                // vehicles parked on the private parking
                transf_t += 1;
            }
            else {
                transf_t = std::numeric_limits<double>::max();
            }
        }

        return transf_t; 
    }
		
protected:
    const TimetableMap& timetable_; 
    const TimetableMap& rtimetable_; 
    const FrequencyMap& frequency_; 
    const FrequencyMap& rfrequency_; 
    std::vector<db_id_t> allowed_transport_modes_;
    std::map< Multimodal::Vertex, db_id_t >& vehicle_nodes_; 
    double walking_speed_; 
    double cycling_speed_; 
    double min_transfer_time_; 
    double car_parking_search_time_; 
    boost::optional<Road::Vertex> private_parking_;
}; 

}

#endif
