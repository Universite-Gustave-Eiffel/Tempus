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

#include <boost/test/unit_test.hpp>

#include "routing_data.hh"
#include "routing_data_builder.hh"
#include "multimodal_graph.hh"

using namespace boost::unit_test ;
using namespace Tempus;

BOOST_AUTO_TEST_SUITE( tempus_graph_builder )

std::string g_db_options = getenv( "TEMPUS_DB_OPTIONS" ) ? getenv( "TEMPUS_DB_OPTIONS" ) : "";
std::string g_db_name = getenv( "TEMPUS_DB_NAME" ) ? getenv( "TEMPUS_DB_NAME" ) : "tempus_test_db";

BOOST_AUTO_TEST_CASE( test_routing_data_builder )
{
    RoutingDataBuilder* builder = RoutingDataBuilderRegistry::instance().builder( "multimodal_graph" );
    BOOST_CHECK( builder );

    TextProgression progression;
    std::unique_ptr<Tempus::RoutingData> g = builder->pg_import( g_db_options + " dbname=" + g_db_name, progression );

    std::unique_ptr<Multimodal::Graph> mm_graph( static_cast<Multimodal::Graph*>(g.release()) );

    std::cout << num_vertices( *mm_graph ) << std::endl;
}

BOOST_AUTO_TEST_CASE( test_routing_data_loading )
{
    TextProgression progression;
    const RoutingData* data = load_routing_data_from_db( "multimodal_graph", g_db_options + " dbname=" + g_db_name, progression );
    BOOST_CHECK( data );
    
    const RoutingData* data2 = load_routing_data_from_db( "multimodal_graph", g_db_options + " dbname=" + g_db_name, progression );
    BOOST_CHECK_EQUAL( data, data2 );

    const Multimodal::Graph* mm_graph = static_cast<const Multimodal::Graph*>( data );
    std::cout << num_vertices( *mm_graph ) << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()

