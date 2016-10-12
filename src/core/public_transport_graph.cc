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

#include "public_transport_graph.hh"

namespace Tempus {
namespace PublicTransport {

void Network::add_agency( const Agency& agency )
{
    agencies_.push_back( agency );
}

void Timetable::assign_sorted_table( const std::vector<TripTime>& table )
{
    table_ = table;
}

void Timetable::assign_sorted_table( std::vector<TripTime>&& table )
{
    table_ = table;
}

void ServiceMap::add( db_id_t service_id, const Date& date )
{
    auto it = map_.find( service_id );
    if ( it == map_.end() ) {
        auto pp = map_.emplace( std::make_pair( service_id, Service() ) );
        it = pp.first;
    }
    it->second.insert( date );
}

bool ServiceMap::is_available_on( db_id_t service_id, const Date& date ) const
{
    auto it = map_.find( service_id );
    if ( it == map_.end() )
        return false;
    return it->second.find( date ) != it->second.end();
}

static bool departure_cmp( const Timetable::TripTime& t1, float t )
{
    return t1.departure_time() < t;
}

std::pair<Timetable::TripTimeIterator, Timetable::TripTimeIterator> Timetable::next_departures( float time_min ) const
{
    auto it = std::lower_bound( table_.begin(), table_.end(), time_min, departure_cmp );
    if ( it == table_.end() )
        return std::make_pair( table_.begin(), table_.begin() );
    float t = it->departure_time();
    while ( it->departure_time() == t )
        it--;
    it++;
    auto it2 = it;
    while ( it2->departure_time() == t )
        it2++;
    return std::make_pair( it, it2 );
}

static bool arrival_cmp( float t, const Timetable::TripTime& t1 )
{
    return t < t1.arrival_time();
}

std::pair<Timetable::TripTimeIterator, Timetable::TripTimeIterator> Timetable::previous_arrivals( float time_min ) const
{
    auto it = std::upper_bound( table_.begin(), table_.end(), time_min, arrival_cmp );
    if ( it == table_.begin() )
        return std::make_pair( table_.begin(), table_.begin() );
    it--;
    float t = it->arrival_time();
    while ( it->arrival_time() == t )
        it--;
    it++;
    auto it2 = it;
    while ( it2->arrival_time() == t )
        it2++;
    return std::make_pair( it, it2 );
}

boost::optional<Timetable::TripTime> next_departure( const Graph& g, const Edge& e, const Date& day, float time )
{
    auto p = g[e].time_table().next_departures( time );
    if ( p.first == p.second )
        return boost::optional<Timetable::TripTime>();
    const GraphProperties& props = get_property( g );
    for ( auto it = p.first; it != p.second; it++ ) {
        if ( props.service_map().is_available_on( it->service_id(), day ) )
            return *it;
    }
    return boost::optional<Timetable::TripTime>();
}

boost::optional<Timetable::TripTime> previous_arrival( const Graph& g, const Edge& e, const Date& day, float time )
{
    auto p = g[e].time_table().previous_arrivals( time );
    if ( p.first == p.second )
        return boost::optional<Timetable::TripTime>();
    const GraphProperties& props = get_property( g );
    for ( auto it = p.first; it != p.second; it++ ) {
        if ( props.service_map().is_available_on( it->service_id(), day ) )
            return *it;
    }
    return boost::optional<Timetable::TripTime>();
}

} // PublicTransport namespace
} // Tempus namespace
