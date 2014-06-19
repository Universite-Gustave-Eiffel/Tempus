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
        Db::Result res( connection_.exec( "SELECT id, name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle" ) );
        graph.clear_constants();

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id=0; // avoid warning "may be used uninitialized"
            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );
            // populate the global variable
            TransportMode t;
            t.db_id( db_id );
            t.name( res[i][1] );
            t.is_public_transport( res[i][2] );
            // gtfs_route_type ??
            if ( ! res[i][4].is_null() ) t.traffic_rules( res[i][4] );
            if ( ! res[i][5].is_null() ) t.speed_rule( static_cast<TransportModeSpeedRule>(res[i][5].as<int>()) );
            if ( ! res[i][6].is_null() ) t.toll_rules( res[i][6] );
            if ( ! res[i][7].is_null() ) t.engine_type( static_cast<TransportModeEngine>(res[i][7].as<int>()) );
            if ( ! res[i][8].is_null() ) t.need_parking( res[i][8] );
            if ( ! res[i][9].is_null() ) t.is_shared( res[i][9] );
            if ( ! res[i][10].is_null() ) t.must_be_returned( res[i][10] );

            graph.add_transport_mode( t );

            progression( static_cast<float>( ( i + 0. ) / res.size() / 2.0 ) );
        }
    }

#if 0 // no need for stored road types anymore ??
    {

        Db::Result res( connection_.exec( "SELECT id, rtname FROM tempus.road_type" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id=0; // avoid warning "may be used uninitialized"
            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );
            // populate the global variable
            RoadType& t = graph.road_types[ db_id ];
            t.db_id( db_id );
            res[i][1] >> t.name;
            // assign the name <-> id map
            graph.road_type_from_name[ t.name ] = t.db_id();

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 2.0 ) + 0.5 ) );
        }
    }
#endif
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
        Db::Result res( connection_.exec( "SELECT id, bifurcation FROM tempus.road_node" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Node node;

            node.db_id( res[i][0] );
            // only overwritten if not null
            node.is_bifurcation( res[i][1] );

            Road::Vertex v = boost::add_vertex( node, road_graph );

            road_nodes_map[ node.db_id() ] = v;
            road_graph[v].vertex( v );

            progression( static_cast<float>( ( i + 0. ) / res.size() / 4.0 ) );
        }
    }

    //
    // check road section consistency
    {
        // look for sections where 'from' or 'to' road ids do not exist
        const std::string q = "SELECT COUNT(*) "
            "FROM "
            "tempus.road_section AS rs "
            "LEFT JOIN tempus.road_node AS rn1 "
            "ON rs.node_from = rn1.id "
            "LEFT JOIN tempus.road_node AS rn2 "
            "ON rs.node_to = rn2.id "
            "WHERE "
            "rn1.id IS NULL "
            "OR "
            "rn2.id IS NULL";

        Db::Result res( connection_.exec( q ) );
        size_t count = 0;
        res[0][0] >> count;
        if ( count ) {
            CERR << "[WARNING]: there are " << count << " road sections with inexistent node_from or node_to" << std::endl;
        }
    }
    {
        // look for cycles
        const std::string q = "SELECT count(*) FROM tempus.road_section WHERE node_from = node_to";
        Db::Result res( connection_.exec( q ) );
        size_t count = 0;
        res[0][0] >> count;
        if ( count ) {
            CERR << "[WARNING]: there are " << count << " road sections with cycles" << std::endl;
        }
    }

    {
        // look for duplicated road sections that do not serve as
        // attachment for any PT stop or POI
        const std::string q = "SELECT COUNT(rs.id) FROM "
            "( "
            "SELECT "
            "	rs1.id "
            "FROM "
            "	tempus.road_section as rs1, "
            "	tempus.road_section as rs2 "
            "WHERE "
            "	rs1.node_from = rs2.node_from "
            "AND "
            "	rs1.node_to = rs2.node_to "
            "AND rs1.id != rs2.id "
            ") AS rs "
            "LEFT JOIN tempus.pt_stop AS pt "
            "ON rs.id = pt.road_section_id "
            "LEFT JOIN tempus.poi AS poi "
            "ON rs.id = poi.road_section_id "
            "WHERE "
            "pt.id IS NULL "
            "and "
            "poi.id IS NULL";

        Db::Result res( connection_.exec( q ) );
        size_t count = 0;
        res[0][0] >> count;
        if ( count ) {
            CERR << "[WARNING]: there are " << count << " duplicated road sections of the same orientation" << std::endl;
        }
    }

    {
        //
        // Get a road section and its opposite, if present
        const std::string qquery = "SELECT "
            "rs1.id, rs1.road_type, rs1.node_from, rs1.node_to, rs1.transport_type_ft, "
            "rs1.transport_type_tf, rs1.length, rs1.car_speed_limit, rs1.car_average_speed, rs1.lane, "
            "rs1.roundabout, rs1.bridge, rs1.tunnel, rs1.ramp, rs1.tollway, "
            "rs2.id, rs2.road_type, "
            "rs2.transport_type_ft, rs2.length, rs2.car_speed_limit, rs2.car_average_speed, rs2.lane, "
            "rs2.roundabout, rs2.bridge, rs2.tunnel, rs2.ramp, rs2.tollway "
            "FROM tempus.road_section AS rs1 "
            "LEFT JOIN tempus.road_section AS rs2 "
            "ON rs1.node_from = rs2.node_to AND rs1.node_to = rs2.node_from ";
        Db::Result res( connection_.exec( qquery ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Section section;
            Road::Section section2;

            section.db_id( res[i][0].as<db_id_t>() );
            BOOST_ASSERT( section.db_id() > 0 );

            if ( !res[i][1].is_null() ) {
                section.road_type = res[i][1].as<db_id_t>();
            }

            db_id_t node_from_id = res[i][2].as<db_id_t>();
            db_id_t node_to_id = res[i][3].as<db_id_t>();
            BOOST_ASSERT( node_from_id > 0 );
            BOOST_ASSERT( node_to_id > 0 );

            int j = 4;
            int transport_type_ft, transport_type_tf;
            res[i][j++] >> transport_type_ft;
            res[i][j++] >> transport_type_tf;
            section.transport_type = transport_type_ft;
            res[i][j++] >> section.length;
            res[i][j++] >> section.car_speed_limit;
            res[i][j++] >> section.car_average_speed;
            res[i][j++] >> section.lane;
            res[i][j++] >> section.is_roundabout;
            res[i][j++] >> section.is_bridge;
            res[i][j++] >> section.is_tunnel;
            res[i][j++] >> section.is_ramp;
            res[i][j++] >> section.is_tollway;

            if ( ! res[i][j].is_null() ) {
                //
                // If the opposite section exists, take it
                section2.db_id( res[i][j++].as<db_id_t>());
                res[i][j++] >> section2.road_type;
                // overwrite transport_type_tf here
                res[i][j++] >> transport_type_tf;
                res[i][j++] >> section2.length;
                res[i][j++] >> section2.car_speed_limit;
                res[i][j++] >> section2.car_average_speed;
                res[i][j++] >> section2.lane;
                res[i][j++] >> section2.is_roundabout;
                res[i][j++] >> section2.is_bridge;
                res[i][j++] >> section2.is_tunnel;
                res[i][j++] >> section2.is_ramp;
                res[i][j++] >> section2.is_tollway;
            }
            else {
                // else create an opposite section from this one
                section2 = section;
                section2.transport_type = transport_type_tf;
            }
            // Assert that corresponding nodes exist
            BOOST_ASSERT_MSG( road_nodes_map.find( node_from_id ) != road_nodes_map.end(),
                              ( boost::format( "Non existing node_from %1% on road_section %2%" ) % node_from_id % section.db_id() ).str().c_str() );
            BOOST_ASSERT_MSG( road_nodes_map.find( node_to_id ) != road_nodes_map.end(),
                              ( boost::format( "Non existing node_to %1% on road_section %2%" ) % node_to_id % section.db_id() ).str().c_str() );
            Road::Vertex v_from = road_nodes_map[ node_from_id ];
            Road::Vertex v_to = road_nodes_map[ node_to_id ];

            if ( v_from == v_to ) {
                CERR << "Cannot add a road cycle from " << v_from << " to " << v_to << endl;
                continue;
            }

            if ( transport_type_ft > 0 ) {
                Road::Edge e;
                bool is_added, found;
                boost::tie( e, found ) = boost::edge( v_from, v_to, road_graph );
                /*if ( found ) {
                    continue;
                }*/

                boost::tie( e, is_added ) = boost::add_edge( v_from, v_to, section, road_graph );
                BOOST_ASSERT( is_added );
                road_graph[e].edge = e;
                // link the road_section to this edge
                road_sections_map[ section.db_id() ] = e;
            }
            if ( transport_type_tf > 0 ) {
                Road::Edge e;
                bool is_added, found;
                boost::tie( e, found ) = boost::edge( v_to, v_from, road_graph );
                /*if ( found ) {
                    continue;
                }*/

                boost::tie( e, is_added ) = boost::add_edge( v_to, v_from, section2, road_graph );
                BOOST_ASSERT( is_added );
                road_graph[e].edge = e;
            }

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.25 ) );
        }
    }

    {
        Db::Result res( connection_.exec( "SELECT id, pnname, provided_transport_types FROM tempus.pt_network" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Network network;

            network.db_id( res[i][0] );
            BOOST_ASSERT( network.db_id() > 0 );
            res[i][1] >> network.name;
            res[i][2] >> network.provided_transport_types;

            graph.network_map[network.db_id()] = network;
            graph.public_transports[network.db_id()] = PublicTransport::Graph();
        }
    }

    {

        Db::Result res( connection_.exec( "select distinct on (n.id) "
                                          "s.network_id, n.id, n.psname, n.location_type, "
                                          "n.parent_station, n.road_section_id, n.zone_id, n.abscissa_road_section "
                                          "from tempus.pt_stop as n, tempus.pt_section as s "
                                          "where s.stop_from = n.id or s.stop_to = n.id" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Stop stop;

            int j = 0;
            Tempus::db_id_t network_id;
            res[i][j++] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( graph.network_map.find( network_id ) != graph.network_map.end() );
            BOOST_ASSERT( graph.public_transports.find( network_id ) != graph.public_transports.end() );
            PublicTransport::Graph& pt_graph = graph.public_transports[network_id];

            stop.db_id( res[i][j++] );
            BOOST_ASSERT( stop.db_id() > 0 );
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
                CERR << "Cannot find road_section of ID " << road_section << ", pt node " << stop.db_id() << " rejected" << endl;
                continue;
            }

            stop.road_edge = road_sections_map[ road_section ];
            // look for an opposite road edge
            {
                Road::Edge opposite_edge;
                bool found;
                boost::tie( opposite_edge, found ) = edge( target( stop.road_edge, graph.road ),
                                                           source( stop.road_edge, graph.road ),
                                                           graph.road );
                if ( found ) {
                    stop.opposite_road_edge = opposite_edge;
                }
                else {
                    stop.opposite_road_edge = stop.road_edge;
                }
            }

            res[i][j++] >> stop.zone_id;
            res[i][j++] >> stop.abscissa_road_section;

            PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
            pt_nodes_map[network_id][ stop.db_id() ] = v;
            pt_graph[v].vertex = v;
            pt_graph[v].graph = &pt_graph;

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.5 ) );
        }
    }

    //
    // For all public transport nodes, add a reference to the attached road section
    Multimodal::Graph::PublicTransportGraphList::iterator it;

    for ( it = graph.public_transports.begin(); it != graph.public_transports.end(); it++ ) {
        PublicTransport::Graph& g = it->second;
        PublicTransport::VertexIterator vi, vi_end;

        for ( boost::tie( vi, vi_end ) = boost::vertices( g ); vi != vi_end; vi++ ) {
            Road::Edge rs = g[ *vi ].road_edge;
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
        Db::Result res( connection_.exec( "SELECT id, poi_type, pname, parking_transport_mode, road_section_id, abscissa_road_section FROM tempus.poi" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            POI poi;

            poi.db_id( res[i][0] );
            BOOST_ASSERT( poi.db_id() > 0 );

            poi.poi_type( static_cast<POI::PoiType>(res[i][1].as<int>()) );
            poi.name( res[i][2] );
            poi.parking_transport_mode( res[i][3] );

            db_id_t road_section_id;
            res[i][4] >> road_section_id;
            BOOST_ASSERT( road_section_id > 0 );

            if ( road_sections_map.find( road_section_id ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section_id << ", poi " << poi.db_id() << " rejected" << endl;
                continue;
            }

            poi.road_edge( road_sections_map[ road_section_id ] );
            // look for an opposite road edge
            {
                Road::Edge opposite_edge;
                bool found;
                boost::tie( opposite_edge, found ) = edge( target( poi.road_edge(), graph.road ),
                                                           source( poi.road_edge(), graph.road ),
                                                           graph.road );
                if ( found ) {
                    poi.opposite_road_edge( opposite_edge );
                }
                else {
                    poi.opposite_road_edge( poi.road_edge() );
                }
            }

            poi.abscissa_road_section( res[i][5] );

            graph.pois[ poi.db_id() ] = poi;
        }
    }

    //
    // For all POIs, add a reference to the attached road section
    Multimodal::Graph::PoiList::iterator pit;

    for ( pit = graph.pois.begin(); pit != graph.pois.end(); pit++ ) {
        Road::Edge rs = pit->second.road_edge();
        graph.road[ rs ].pois.push_back( &pit->second ); 
    }

    //
    // By default, all public transport networks are part of the selected subset
    graph.public_transports.select_all();

    progression( 1.0, /* finished = */ true );

}

Road::Restrictions PQImporter::import_turn_restrictions( const Road::Graph& graph )
{
    Road::Restrictions restrictions( graph );

    // temp restriction storage
    typedef std::map<db_id_t, Road::Restriction::EdgeSequence> EdgesMap;
    EdgesMap edges_map;

    {
        Db::Result res( connection_.exec( "SELECT id, sections FROM tempus.road_restriction" ) );

        std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
        Road::EdgeIterator it, it_end;
        boost::tie( it, it_end ) = boost::edges( graph );

        for ( ; it != it_end; ++it ) {
            road_sections_map[ graph[ *it ].db_id() ] = *it;
        }

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id;

            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );

            std::string array_str;
            res[i][1] >> array_str;
            // array: {a,b,c}

            // trim '{}'
            std::istringstream array_sub( array_str.substr( 1, array_str.size()-2 ) );

            // parse each array element
            std::string n_str;
            bool invalid = false;

            Road::Restriction::EdgeSequence edges;
            while ( std::getline( array_sub, n_str, ',' ) ) {
                std::istringstream n_stream( n_str );
                db_id_t id;
                n_stream >> id;

                if ( road_sections_map.find( id ) == road_sections_map.end() ) {
                    CERR << "Cannot find road_section of ID " << id << ", road_restriction of ID " << db_id << " rejected" << endl;
                    invalid = true;
                    break;
                }

                edges.push_back( road_sections_map[id] );
            }

            if ( invalid ) {
                continue;
            }

            // store the current road_restriction
            edges_map[ db_id ] = edges;
        }
    }

    // get restriction costs
    {
        Db::Result res( connection_.exec( "SELECT id, restriction_id, transport_types, cost FROM tempus.road_restriction_cost" ) );
        std::map<db_id_t, Road::Restriction::CostPerTransport> costs;
        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t id, restr_id;
            int transports;
            double cost;

            res[i][0] >> id;
            res[i][1] >> restr_id;
            res[i][2] >> transports;
            res[i][3] >> cost;

            if ( edges_map.find( restr_id ) == edges_map.end() ) {
                CERR << "Cannot find road_restriction of ID " << restr_id << " for restriction_cost of ID " << id << endl;
                continue;
            }
            // add cost to the map
            costs[restr_id][transports] = cost;
        }
        for ( std::map<db_id_t, Road::Restriction::CostPerTransport>::const_iterator it = costs.begin();
              it != costs.end(); ++it ) {
            restrictions.add_restriction( it->first, edges_map[ it->first ], it->second );
        }
    }
    return restrictions;
}
}
