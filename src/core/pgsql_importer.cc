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

#include <libpq-fe.h>

#include <iostream>

#include <boost/mpl/vector.hpp>
#include <boost/format.hpp>

#include "pgsql_importer.hh"

using namespace std;

namespace Tempus {
PQImporter::PQImporter( const std::string& pg_options ) :
    connection_( pg_options )
{
}

Db::Result PQImporter::query( const std::string& query_str )
{
    return connection_.exec( query_str );
}

void PQImporter::import_constants( Multimodal::Graph& graph, ProgressionCallback& progression )
{
    {
        Db::Result res( connection_.exec( "SELECT id, parent_id, ttname, need_parking, need_station, need_return, need_network FROM tempus.transport_type" ) );
        graph.transport_types.clear();
        graph.transport_type_from_name.clear();
        graph.road_types.clear();
        graph.road_type_from_name.clear();

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id=0; // avoid warning "may be used uninitialized"
            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );
            // populate the global variable
            TransportType& t = graph.transport_types[ db_id ];
            t.db_id = db_id;
            // default parent_id == null
            t.parent_id = 0;
            res[i][1] >> t.parent_id; // <- if the field is null, t.parent_id is kept untouched
            res[i][2] >> t.name;
            res[i][3] >> t.need_parking;
            res[i][4] >> t.need_station;
            res[i][5] >> t.need_return;
            res[i][6] >> t.need_network;

            // assign the name <-> id map
            graph.transport_type_from_name[ t.name ] = t.db_id;

            progression( static_cast<float>( ( i + 0. ) / res.size() / 2.0 ) );
        }
    }

    {

        Db::Result res( connection_.exec( "SELECT id, rtname FROM tempus.road_type" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id=0; // avoid warning "may be used uninitialized"
            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );
            // populate the global variable
            RoadType& t = graph.road_types[ db_id ];
            t.db_id = db_id;
            res[i][1] >> t.name;
            // assign the name <-> id map
            graph.road_type_from_name[ t.name ] = t.db_id;

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 2.0 ) + 0.5 ) );
        }
    }
}

///
/// Function used to import the road and public transport graphs from a PostgreSQL database.
void PQImporter::import_graph( Multimodal::Graph& graph, ProgressionCallback& progression )
{
    Road::Graph& road_graph = graph.road;
    road_graph.clear();
    graph.network_map.clear();
    graph.public_transports.clear();
    graph.pois.clear();

    // locally maps db ID to Node or Section
    std::map<Tempus::db_id_t, Road::Vertex> road_nodes_map;
    std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
    // network_id -> pt_node_id -> vertex
    std::map<Tempus::db_id_t, std::map<Tempus::db_id_t, PublicTransport::Vertex> > pt_nodes_map;

    {
        Db::Result res( connection_.exec( "SELECT id, junction, bifurcation FROM tempus.road_node" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Node node;

            node.db_id = res[i][0].as<db_id_t>();
            node.is_junction = false;
            node.is_bifurcation = false;
            node.vertex =  Road::Vertex();
            BOOST_ASSERT( node.db_id > 0 );
            // only overwritten if not null
            res[i][1] >> node.is_junction;
            res[i][2] >> node.is_bifurcation;

            Road::Vertex v = boost::add_vertex( node, road_graph );

            road_nodes_map[ node.db_id ] = v;
            road_graph[v].vertex = v;

            progression( static_cast<float>( ( i + 0. ) / res.size() / 4.0 ) );
        }
    }

    {
        const std::string qquery = "SELECT id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, "
                                   "car_average_speed, road_name, lane, "
                                   "roundabout, bridge, tunnel, ramp, tollway FROM tempus.road_section";
        Db::Result res( connection_.exec( qquery ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Section section;

            res[i][0] >> section.db_id;
            BOOST_ASSERT( section.db_id > 0 );

            if ( !res[i][1].is_null() ) {
                section.road_type = res[i][1].as<db_id_t>();
            }

            db_id_t node_from_id = res[i][2].as<db_id_t>();
            db_id_t node_to_id = res[i][3].as<db_id_t>();
            BOOST_ASSERT( node_from_id > 0 );
            BOOST_ASSERT( node_to_id > 0 );

            int j = 4;
            res[i][j++] >> section.transport_type_ft;
            res[i][j++] >> section.transport_type_tf;
            res[i][j++] >> section.length;
            res[i][j++] >> section.car_speed_limit;
            res[i][j++] >> section.car_average_speed;
            res[i][j++] >> section.road_name;
            res[i][j++] >> section.lane;
            res[i][j++] >> section.is_roundabout;
            res[i][j++] >> section.is_bridge;
            res[i][j++] >> section.is_tunnel;
            res[i][j++] >> section.is_ramp;
            res[i][j++] >> section.is_tollway;

            // Assert that corresponding nodes exist
            BOOST_ASSERT_MSG( road_nodes_map.find( node_from_id ) != road_nodes_map.end(),
                              ( boost::format( "Non existing node_from %1% on road_section %2%" ) % node_from_id % section.db_id ).str().c_str() );
            BOOST_ASSERT_MSG( road_nodes_map.find( node_to_id ) != road_nodes_map.end(),
                              ( boost::format( "Non existing node_to %1% on road_section %2%" ) % node_to_id % section.db_id ).str().c_str() );
            Road::Vertex v_from = road_nodes_map[ node_from_id ];
            Road::Vertex v_to = road_nodes_map[ node_to_id ];

            if ( v_from == v_to ) {
                CERR << "Cannot add a road cycle from " << v_from << " to " << v_to << endl;
                continue;
            }

            Road::Edge e;
            bool is_added, found;
            boost::tie( e, found ) = boost::edge( v_from, v_to, road_graph );

            if ( found ) {
                CERR << "Edge " << e << " already exists" << endl;
                continue;
            }

            boost::tie( e, is_added ) = boost::add_edge( v_from, v_to, section, road_graph );
            BOOST_ASSERT( is_added );
            road_graph[e].edge = e;
            road_sections_map[ section.db_id ] = e;

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.25 ) );
        }
    }

    {
        Db::Result res( connection_.exec( "SELECT id, pnname, provided_transport_types FROM tempus.pt_network" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Network network;

            res[i][0] >> network.db_id;
            BOOST_ASSERT( network.db_id > 0 );
            res[i][1] >> network.name;
            res[i][2] >> network.provided_transport_types;

            graph.network_map[network.db_id] = network;
            graph.public_transports[network.db_id] = PublicTransport::Graph();
        }
    }

    {

        Db::Result res( connection_.exec( "SELECT DISTINCT s.network_id, n.id, n.psname, n.location_type, n.parent_station, n.road_section_id, n.zone_id, n.abscissa_road_section FROM tempus.pt_stop as n JOIN tempus.pt_section as s ON s.stop_from = n.id OR s.stop_to = n.id ORDER by n.id" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Stop stop;

            int j = 0;
            Tempus::db_id_t network_id;
            res[i][j++] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( graph.network_map.find( network_id ) != graph.network_map.end() );
            BOOST_ASSERT( graph.public_transports.find( network_id ) != graph.public_transports.end() );
            PublicTransport::Graph& pt_graph = graph.public_transports[network_id];

            res[i][j++] >> stop.db_id;
            BOOST_ASSERT( stop.db_id > 0 );
            res[i][j++] >> stop.name;
            res[i][j++] >> stop.is_station;

            stop.has_parent = false;
            int parent_station = 0; // initilisation avoid warning
            res[i][j++] >> parent_station;

            if ( pt_nodes_map[network_id].find( parent_station ) != pt_nodes_map[network_id].end() ) {
                stop.parent_station = pt_nodes_map[network_id][ parent_station ];
                stop.has_parent = true;
            }

            Tempus::db_id_t road_section;
            res[i][j++] >> road_section;
            BOOST_ASSERT( road_section > 0 );

            if ( road_sections_map.find( road_section ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section << ", pt node " << stop.db_id << " rejected" << endl;
                continue;
            }

            stop.road_section = road_sections_map[ road_section ];

            res[i][j++] >> stop.zone_id;
            res[i][j++] >> stop.abscissa_road_section;

            PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
            pt_nodes_map[network_id][ stop.db_id ] = v;
            pt_graph[v].vertex = v;
            pt_graph[v].graph = &pt_graph;

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.5 ) );
        }
    }

    //
    // For all public transport nodes, add a reference to it to the attached road section
    Multimodal::Graph::PublicTransportGraphList::iterator it;

    for ( it = graph.public_transports.begin(); it != graph.public_transports.end(); it++ ) {
        PublicTransport::Graph& g = it->second;
        PublicTransport::VertexIterator vi, vi_end;

        for ( boost::tie( vi, vi_end ) = boost::vertices( g ); vi != vi_end; vi++ ) {
            Road::Edge rs = g[ *vi ].road_section;
            road_graph[ rs ].stops.push_back( &g[*vi] );
        }
    }

    {
        Db::Result res( connection_.exec( "SELECT network_id, stop_from, stop_to FROM tempus.pt_section" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Section section;

            Tempus::db_id_t network_id;
            res[i][0] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( graph.public_transports.find( network_id ) != graph.public_transports.end() );
            PublicTransport::Graph& pt_graph = graph.public_transports[ network_id ];

            Tempus::db_id_t stop_from_id, stop_to_id;
            res[i][1] >> stop_from_id;
            res[i][2] >> stop_to_id;
            BOOST_ASSERT( stop_from_id > 0 );
            BOOST_ASSERT( stop_to_id > 0 );

            if ( pt_nodes_map[network_id].find( stop_from_id ) == pt_nodes_map[network_id].end() ) {
                CERR << "Cannot find 'from' node of ID " << stop_from_id << endl;
                continue;
            }

            if ( pt_nodes_map[network_id].find( stop_to_id ) == pt_nodes_map[network_id].end() ) {
                CERR << "Cannot find 'to' node of ID " << stop_to_id << endl;
                continue;
            }

            PublicTransport::Vertex stop_from = pt_nodes_map[network_id][ stop_from_id ];
            PublicTransport::Vertex stop_to = pt_nodes_map[network_id][ stop_to_id ];
            PublicTransport::Edge e;
            bool is_added;
            boost::tie( e, is_added ) = boost::add_edge( stop_from, stop_to, pt_graph );
            BOOST_ASSERT( is_added );
            pt_graph[e].edge = e;
            pt_graph[e].graph = &pt_graph;
            pt_graph[e].network_id = network_id;
            pt_graph[e].stop_from = stop_from_id;
            pt_graph[e].stop_to = stop_to_id;

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.75 ) );
        }
    }

    {
        Db::Result res( connection_.exec( "SELECT id, poi_type, pname, parking_transport_type, road_section_id, abscissa_road_section FROM tempus.poi" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            POI poi;

            res[i][0] >> poi.db_id;
            BOOST_ASSERT( poi.db_id > 0 );

            res[i][1] >> poi.poi_type;
            res[i][2] >> poi.name;
            res[i][3] >> poi.parking_transport_type;

            db_id_t road_section_id;
            res[i][4] >> road_section_id;
            BOOST_ASSERT( road_section_id > 0 );

            if ( road_sections_map.find( road_section_id ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section_id << ", poi " << poi.db_id << " rejected" << endl;
                continue;
            }

            poi.road_section = road_sections_map[ road_section_id ];

            res[i][5] >> poi.abscissa_road_section;

            graph.pois[ poi.db_id ] = poi;
        }
    }

    //
    // For all POIs, add a reference to it to the attached road section
    Multimodal::Graph::PoiList::iterator pit;

    for ( pit = graph.pois.begin(); pit != graph.pois.end(); pit++ ) {
        Road::Edge rs = pit->second.road_section;
        graph.road[ rs ].pois.push_back( &pit->second );
    }

    //
    // By default, all public transport networks are part of the selected subset
    graph.public_transports.select_all();

    progression( 1.0, /* finished = */ true );

}

Road::Restrictions PQImporter::import_turn_restrictions( const Road::Graph& graph )
{
    Road::Restrictions restrictions;
    restrictions.road_graph = &graph;

    Db::Result res( connection_.exec( "SELECT id, road_section, road_cost, transport_types FROM tempus.road_road" ) );

    std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
    Road::EdgeIterator it, it_end;
    boost::tie( it, it_end ) = boost::edges( graph );

    for ( ; it != it_end; ++it ) {
        road_sections_map[ graph[ *it ].db_id ] = *it;
    }


    for ( size_t i = 0; i < res.size(); i++ ) {
        Road::Road road_road;

        res[i][0] >> road_road.db_id;
        BOOST_ASSERT( road_road.db_id > 0 );

        std::string array_str;
        res[i][1] >> array_str;
        // array: {a,b,c}

        // trim '{}'
        std::istringstream array_sub( array_str.substr( 1, array_str.size()-2 ) );

        // parse each array element
        std::string n_str;
        bool invalid = false;

        while ( std::getline( array_sub, n_str, ',' ) ) {
            std::istringstream n_stream( n_str );
            db_id_t id;
            n_stream >> id;

            if ( road_sections_map.find( id ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << id << ", road_road of ID " << road_road.db_id << " rejected" << endl;
                invalid = true;
                break;
            }

            road_road.road_sections.push_back( road_sections_map[id] );
        }

        if ( invalid ) {
            continue;
        }

        res[i][2] >> road_road.cost;

        res[i][2] >> road_road.transport_types;

        restrictions.restrictions.push_back( road_road );
    }

    return restrictions;
}
}
