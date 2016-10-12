/**
 *   Copyright (C) 2016 Oslandia <infos@oslandia.com>
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

#include <boost/test/unit_test.hpp>

#include "public_transport_graph.hh"

using namespace boost::unit_test ;
using namespace Tempus;

BOOST_AUTO_TEST_SUITE( timetable )

BOOST_AUTO_TEST_CASE( test_timetable )
{
    PublicTransport::Timetable tt;
    std::vector<PublicTransport::Timetable::TripTime> v;
    v.emplace_back( 12.0, 13.0, 1, 1 );
    v.emplace_back( 12.0, 13.0, 1, 2 );
    v.emplace_back( 13.5, 14.0, 3, 2 );
    v.emplace_back( 14.5, 15.0, 3, 2 );
    tt.assign_sorted_table( v );

    {
        auto p = tt.next_departures( 10.0 );
        BOOST_CHECK( p.first != p.second );
        int i = 0;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 2 );
    }

    {
        auto p = tt.next_departures( 12.0 );
        BOOST_CHECK( p.first != p.second );
        int i = 0;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 2 );
    }

    {
        auto p = tt.next_departures( 12.5 );
        BOOST_CHECK( p.first != p.second );
        int i = 2;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 3 );
    }

    {
        auto p = tt.next_departures( 15.0 );
        BOOST_CHECK( p.first == p.second );
    }

    {
        auto p = tt.previous_arrivals( 16.0 );
        BOOST_CHECK( p.first != p.second );
        int i = 3;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 4 );
    }
    {
        auto p = tt.previous_arrivals( 15.0 );
        BOOST_CHECK( p.first != p.second );
        int i = 3;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 4 );
    }
    {
        auto p = tt.previous_arrivals( 13.4 );
        BOOST_CHECK( p.first != p.second );
        int i = 0;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 2 );
    }
    {
        auto p = tt.previous_arrivals( 13.0 );
        BOOST_CHECK( p.first != p.second );
        int i = 0;
        for ( auto it = p.first; it != p.second; it++ ) {
            BOOST_CHECK_EQUAL( it->departure_time(), v[i].departure_time() );
            BOOST_CHECK_EQUAL( it->arrival_time(), v[i].arrival_time() );
            BOOST_CHECK_EQUAL( it->trip_id(), v[i].trip_id() );
            BOOST_CHECK_EQUAL( it->service_id(), v[i].service_id() );
            i++;
        }
        BOOST_CHECK_EQUAL( i, 2 );
    }
    {
        auto p = tt.previous_arrivals( 12.0 );
        BOOST_CHECK( p.first == p.second );
    }
}

BOOST_AUTO_TEST_SUITE_END()

