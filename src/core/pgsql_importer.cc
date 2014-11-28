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

void PQImporter::import_constants( Multimodal::Graph& graph, ProgressionCallback& progression, const std::string& /*schema_name*/ )
{
    Multimodal::Graph::TransportModes modes;
    {
        Db::Result res( connection_.exec( "SELECT id, name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle FROM tempus.transport_mode" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t db_id=0; // avoid warning "may be used uninitialized"
            res[i][0] >> db_id;
            BOOST_ASSERT( db_id > 0 );
            // populate the global variable
            TransportMode t;
            t.set_db_id( db_id );
            t.set_name( res[i][1] );
            t.set_is_public_transport( res[i][2] );
            // gtfs_route_type ??
            if ( ! res[i][4].is_null() ) t.set_traffic_rules( res[i][4] );
            if ( ! res[i][5].is_null() ) t.set_speed_rule( static_cast<TransportModeSpeedRule>(res[i][5].as<int>()) );
            if ( ! res[i][6].is_null() ) t.set_toll_rules( res[i][6] );
            if ( ! res[i][7].is_null() ) t.set_engine_type( static_cast<TransportModeEngine>(res[i][7].as<int>()) );
            if ( ! res[i][8].is_null() ) t.set_need_parking( res[i][8] );
            if ( ! res[i][9].is_null() ) t.set_is_shared( res[i][9] );
            if ( ! res[i][10].is_null() ) t.set_must_be_returned( res[i][10] );

            modes[t.db_id()] = t;

            progression( static_cast<float>( ( i + 0. ) / res.size() / 2.0 ) );
        }
        graph.set_transport_modes( modes );
    }
}

///
/// Function used to import the road and public transport graphs from a PostgreSQL database.
std::auto_ptr<Multimodal::Graph> PQImporter::import_graph( ProgressionCallback& progression, bool consistency_check, const std::string& schema_name )
{
    Road::Graph* road_graph = new Road::Graph();
    std::auto_ptr<Road::Graph> sm_road_graph( road_graph );

    if ( consistency_check ) {
        //
        // check road section consistency
        {
            // look for sections where 'from' or 'to' road ids do not exist
            const std::string q = (boost::format("SELECT COUNT(*) "
                                                 "FROM "
                                                 "%1%.road_section AS rs "
                                                 "LEFT JOIN %1%.road_node AS rn1 "
                                                 "ON rs.node_from = rn1.id "
                                                 "LEFT JOIN %1%.road_node AS rn2 "
                                                 "ON rs.node_to = rn2.id "
                                                 "WHERE "
                                                 "rn1.id IS NULL "
                                                 "OR "
                                                 "rn2.id IS NULL") % schema_name ).str();

            Db::Result res( connection_.exec( q ) );
            size_t count = 0;
            res[0][0] >> count;
            if ( count ) {
                CERR << "[WARNING]: there are " << count << " road sections with inexistent node_from or node_to" << std::endl;
            }
        }
        {
            // look for cycles
            const std::string q = (boost::format("SELECT count(*) FROM %1%.road_section WHERE node_from = node_to") % schema_name).str();
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
            const std::string q = (boost::format("SELECT COUNT(rs.id) FROM "
                                                 "( "
                                                 "SELECT "
                                                 "	rs1.id "
                                                 "FROM "
                                                 "	%1%.road_section as rs1, "
                                                 "	%1%.road_section as rs2 "
                                                 "WHERE "
                                                 "	rs1.node_from = rs2.node_from "
                                                 "AND "
                                                 "	rs1.node_to = rs2.node_to "
                                                 "AND rs1.id != rs2.id "
                                                 ") AS rs "
                                                 "LEFT JOIN %1%.pt_stop AS pt "
                                                 "ON rs.id = pt.road_section_id "
                                                 "LEFT JOIN %1%.poi AS poi "
                                                 "ON rs.id = poi.road_section_id "
                                                 "WHERE "
                                                 "pt.id IS NULL "
                                                 "and "
                                                 "poi.id IS NULL") % schema_name).str();

            Db::Result res( connection_.exec( q ) );
            size_t count = 0;
            res[0][0] >> count;
            if ( count ) {
                CERR << "[WARNING]: there are " << count << " duplicated road sections of the same orientation" << std::endl;
            }
        }

        {
            // look for PT stops that are not referenced by any PT sections, or stop times
            const std::string q = (boost::format("SELECT COUNT(*) FROM %1%.pt_stop AS p "
                                                 "left join"
                                                 "    %1%.pt_section as s1 "
                                                 "on"
                                                 "    p.id = s1.stop_from "
                                                 "left join"
                                                 "    %1%.pt_section as s2 "
                                                 "on"
                                                 "    p.id = s2.stop_to "
                                                 "left join"
                                                 "    %1%.pt_stop_time "
                                                 "on p.id = stop_id "
                                                 "left join"
                                                 "    %1%.pt_stop as pp "
                                                 "on p.id = pp.parent_station "
                                                 "where"
                                                 "    s1.stop_from is null "
                                                 "and s2.stop_to is null "
                                                 "and stop_id is null "
                                                 "and pp.parent_station is null" ) % schema_name ).str();
            Db::Result res( connection_.exec( q ) );
            size_t count = 0;
            res[0][0] >> count;
            if ( count ) {
                CERR << "[WARNING]: there are " << count << " unreferenced public transport stops" << std::endl;
            }
        }
    }

    // locally maps db ID to Node or Section
    std::map<Tempus::db_id_t, Road::Vertex> road_nodes_map;
    std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
    // network_id -> pt_node_id -> vertex
    std::map<Tempus::db_id_t, std::map<Tempus::db_id_t, PublicTransport::Vertex> > pt_nodes_map;

    //------------------
    //   Road nodes
    //------------------
    {
        Db::Result res( connection_.exec( (boost::format("SELECT id, bifurcation, st_x(geom), st_y(geom), st_z(geom) FROM %1%.road_node") % schema_name).str() ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Node node;

            node.set_db_id( res[i][0] );
            // only overwritten if not null
            node.set_is_bifurcation( res[i][1] );

            Point3D p;
            p.set_x( res[i][2] );
            p.set_y( res[i][3] );
            p.set_z( res[i][4] );
            node.set_coordinates(p);

            Road::Vertex v = boost::add_vertex( node, *road_graph );

            road_nodes_map[ node.db_id() ] = v;
            (*road_graph)[v].set_vertex( v );

            progression( static_cast<float>( ( i + 0. ) / res.size() / 4.0 ) );
        }
    }

    //------------------
    //   Road sections
    //------------------
    {
        //
        // Get a road section and its opposite, if present
        const std::string qquery = (boost::format("SELECT "
                                                  "rs1.id, rs1.road_type, rs1.node_from, rs1.node_to, rs1.traffic_rules_ft, "
                                                  "rs1.traffic_rules_tf, rs1.length, rs1.car_speed_limit, rs1.lane, "
                                                  "rs1.roundabout, rs1.bridge, rs1.tunnel, rs1.ramp, rs1.tollway, rs1.road_name, "
                                                  "rs2.id, rs2.road_type, "
                                                  "rs2.traffic_rules_ft, rs2.length, rs2.car_speed_limit, rs2.lane, "
                                                  "rs2.roundabout, rs2.bridge, rs2.tunnel, rs2.ramp, rs2.tollway "
                                                  "FROM %1%.road_section AS rs1 "
                                                  "LEFT JOIN %1%.road_section AS rs2 "
                                                  "ON rs1.node_from = rs2.node_to AND rs1.node_to = rs2.node_from ") % schema_name).str();
        Db::Result res( connection_.exec( qquery ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Road::Section section;
            Road::Section section2;

            section.set_db_id( res[i][0].as<db_id_t>() );
            BOOST_ASSERT( section.db_id() > 0 );

            if ( !res[i][1].is_null() ) {
                section.set_road_type( static_cast<Road::RoadType>(res[i][1].as<int>()) );
            }

            db_id_t node_from_id = res[i][2].as<db_id_t>();
            db_id_t node_to_id = res[i][3].as<db_id_t>();
            BOOST_ASSERT( node_from_id > 0 );
            BOOST_ASSERT( node_to_id > 0 );

            int j = 4;
            int traffic_rules_ft = res[i][j++];
            int traffic_rules_tf = res[i][j++];
            section.set_traffic_rules( traffic_rules_ft );
            section.set_length( res[i][j++] );
            section.set_car_speed_limit( res[i][j++] );
            section.set_lane( res[i][j++] );
            section.set_is_roundabout( res[i][j++] );
            section.set_is_bridge( res[i][j++] );
            section.set_is_tunnel( res[i][j++] );
            section.set_is_ramp( res[i][j++] );
            section.set_is_tollway( res[i][j++] );
            section.set_road_name( res[i][j++] );

            if ( ! res[i][j].is_null() ) {
                //
                // If the opposite section exists, take it
                section2.set_db_id( res[i][j++].as<db_id_t>());
                section2.set_road_type( static_cast<Road::RoadType>(res[i][j++].as<int>()) );
                // overwrite transport_type_tf here
                traffic_rules_tf = res[i][j++];
                section.set_length( res[i][j++] );
                section.set_car_speed_limit( res[i][j++] );
                section.set_lane( res[i][j++] );
                section.set_is_roundabout( res[i][j++] );
                section.set_is_bridge( res[i][j++] );
                section.set_is_tunnel( res[i][j++] );
                section.set_is_ramp( res[i][j++] );
                section.set_is_tollway( res[i][j++] );
            }
            else {
                // else create an opposite section from this one
                section2 = section;
                section2.set_traffic_rules( traffic_rules_tf );
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

            if ( traffic_rules_ft > 0 ) {
                Road::Edge e;
                bool is_added, found;
                boost::tie( e, found ) = boost::edge( v_from, v_to, *road_graph );
                /*if ( found ) {
                    continue;
                }*/

                boost::tie( e, is_added ) = boost::add_edge( v_from, v_to, section, *road_graph );
                BOOST_ASSERT( is_added );
                (*road_graph)[e].set_edge( e );
                // link the road_section to this edge
                road_sections_map[ section.db_id() ] = e;
            }
            if ( traffic_rules_tf > 0 ) {
                Road::Edge e;
                bool is_added, found;
                boost::tie( e, found ) = boost::edge( v_to, v_from, *road_graph );
                /*if ( found ) {
                    continue;
                }*/

                boost::tie( e, is_added ) = boost::add_edge( v_to, v_from, section2, *road_graph );
                BOOST_ASSERT( is_added );
                (*road_graph)[e].set_edge( e );
            }

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.25 ) );
        }
    }

    // build the multimodal graph
    std::auto_ptr<Multimodal::Graph> graph( new Multimodal::Graph( sm_road_graph ) );
    road_graph = &graph->road();

    boost::ptr_map<db_id_t, PublicTransport::Graph> pt_graphs;
    Multimodal::Graph::NetworkMap networks;
    {
        Db::Result res( connection_.exec( "SELECT id, pnname FROM tempus.pt_network" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Network network;

            network.set_db_id( res[i][0] );
            BOOST_ASSERT( network.db_id() > 0 );
            network.set_name( res[i][1] );

            networks[network.db_id()] = network;
            pt_graphs.insert(network.db_id(), std::auto_ptr<PublicTransport::Graph>(new PublicTransport::Graph()));
        }
    }

    {

        Db::Result res( connection_.exec( (boost::format("select distinct on (n.id) "
                                                         "s.network_id, n.id, n.name, n.location_type, "
                                                         "n.parent_station, n.road_section_id, n.zone_id, n.abscissa_road_section "
                                                         ",st_x(n.geom), st_y(n.geom), st_z(n.geom) "
                                                         "from %1%.pt_stop as n, %1%.pt_section as s "
                                                         "where s.stop_from = n.id or s.stop_to = n.id") % schema_name).str() ) );

        for ( size_t i = 0; i < res.size(); i++ ) {

            int j = 0;
            Tempus::db_id_t network_id;
            res[i][j++] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( networks.find( network_id ) != networks.end() );
            BOOST_ASSERT( pt_graphs.find( network_id ) != pt_graphs.end() );
            PublicTransport::Graph& pt_graph = pt_graphs[network_id];
            PublicTransport::Stop stop;

            stop.set_db_id( res[i][j++] );
            BOOST_ASSERT( stop.db_id() > 0 );
            stop.set_name( res[i][j++] );
            stop.set_is_station( res[i][j++] );

            {
                Db::Value v(res[i][j++]);
                if ( ! v.is_null() ) {
                    Tempus::db_id_t p_id = v;
                    if ( pt_nodes_map[network_id].find( p_id ) != pt_nodes_map[network_id].end() ) {
                        stop.set_parent_station( pt_nodes_map[network_id][ p_id ] );
                    }
                }
            }

            Tempus::db_id_t road_section;
            res[i][j++] >> road_section;
            BOOST_ASSERT( road_section > 0 );

            if ( road_sections_map.find( road_section ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section << ", pt node " << stop.db_id() << " rejected" << endl;
                continue;
            }

            stop.set_road_edge( road_sections_map[ road_section ] );
            // look for an opposite road edge
            {
                Road::Edge opposite_edge;
                bool found;
                boost::tie( opposite_edge, found ) = edge( target( stop.road_edge(), *road_graph ),
                                                           source( stop.road_edge(), *road_graph ),
                                                           *road_graph );
                if ( found && (graph->road()[stop.road_edge()].db_id() == graph->road()[opposite_edge].db_id()) ) {
                    stop.set_opposite_road_edge( opposite_edge );
                }
            }

            stop.set_zone_id( res[i][j++] );
            stop.set_abscissa_road_section( res[i][j++] );

            Point3D p;
            p.set_x( res[i][j++] );
            p.set_y( res[i][j++] );
            p.set_z( res[i][j++] );
            stop.set_coordinates(p);

            PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
            pt_nodes_map[network_id][ stop.db_id() ] = v;
            pt_graph[v].set_vertex( v );
            pt_graph[v].set_graph( &pt_graph );

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.5 ) );
        }
    }

    //
    // For all public transport nodes, add a reference to the attached road section
    boost::ptr_map<db_id_t, PublicTransport::Graph>::iterator it;

    for ( it = pt_graphs.begin(); it != pt_graphs.end(); it++ ) {
        PublicTransport::Graph& g = *it->second;
        PublicTransport::VertexIterator vi, vi_end;

        for ( boost::tie( vi, vi_end ) = boost::vertices( g ); vi != vi_end; vi++ ) {
            Road::Edge rs = g[ *vi ].road_edge();
            (*road_graph)[ rs ].add_stop_ref( &g[*vi] );
            // add a ref to the opposite road edge, if any
            if (g[*vi].opposite_road_edge()) {
                rs = g[*vi].opposite_road_edge().get();
                (*road_graph)[rs].add_stop_ref( &g[*vi] );
            }
        }
    }

    {
        Db::Result res( connection_.exec( (boost::format("SELECT network_id, stop_from, stop_to FROM %1%.pt_section") % schema_name).str() ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            Tempus::db_id_t network_id;
            res[i][0] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( pt_graphs.find( network_id ) != pt_graphs.end() );
            PublicTransport::Graph& pt_graph = pt_graphs[ network_id ];

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
            pt_graph[e].set_edge( e );
            pt_graph[e].set_graph( &pt_graph );
            pt_graph[e].set_network_id( network_id );

            progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.75 ) );
        }
    }

    //-------------
    //    POI
    //-------------
    boost::ptr_map<db_id_t, POI> pois;
    {
        Db::Result res( connection_.exec( (boost::format("SELECT id, poi_type, name, parking_transport_modes, road_section_id, abscissa_road_section "
                                                         ", st_x(geom), st_y(geom), st_z(geom) FROM %1%.poi") % schema_name).str() ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            POI poi;

            poi.set_db_id( res[i][0] );
            BOOST_ASSERT( poi.db_id() > 0 );

            poi.set_poi_type( static_cast<POI::PoiType>(res[i][1].as<int>()) );
            poi.set_name( res[i][2] );

            poi.set_parking_transport_modes( res[i][3].as< std::vector<db_id_t> >() );

            db_id_t road_section_id;
            res[i][4] >> road_section_id;
            BOOST_ASSERT( road_section_id > 0 );

            if ( road_sections_map.find( road_section_id ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section_id << ", poi " << poi.db_id() << " rejected" << endl;
                continue;
            }

            poi.set_road_edge( road_sections_map[ road_section_id ] );
            // look for an opposite road edge
            {
                Road::Edge opposite_edge;
                bool found;
                boost::tie( opposite_edge, found ) = edge( target( poi.road_edge(), *road_graph ),
                                                           source( poi.road_edge(), *road_graph ),
                                                           *road_graph );
                if ( found && (graph->road()[poi.road_edge()].db_id() == graph->road()[opposite_edge].db_id()) ) {
                    poi.set_opposite_road_edge( opposite_edge );
                }
            }

            poi.set_abscissa_road_section( res[i][5] );

            Point3D p;
            p.set_x( res[i][6] );
            p.set_y( res[i][7] );
            p.set_z( res[i][8] );
            poi.set_coordinates( p );

            pois.insert( poi.db_id(), std::auto_ptr<POI>(new POI( poi )) );
        }
    }

    //
    // For all POIs, add a reference to the attached road section
    Multimodal::Graph::PoiList::iterator pit;

    for ( pit = pois.begin(); pit != pois.end(); pit++ ) {
        Road::Edge rs = pit->second->road_edge();
        (*road_graph)[ rs ].add_poi_ref( &*pit->second );
        if ( pit->second->opposite_road_edge() ) {
            rs = pit->second->opposite_road_edge().get();
            (*road_graph)[ rs ].add_poi_ref( &*pit->second );
        }
    }

    /// assign to the multimodal graph
    graph->set_network_map( networks );
    graph->set_public_transports( pt_graphs );
    graph->set_pois( pois );

    progression( 1.0, /* finished = */ true );

    return graph;
}

Road::Restrictions PQImporter::import_turn_restrictions( const Road::Graph& graph, const std::string& schema_name )
{
    Road::Restrictions restrictions( graph );

    // temp restriction storage
    typedef std::map<db_id_t, Road::Restriction::EdgeSequence> EdgesMap;
    EdgesMap edges_map;

    {
        Db::Result res( connection_.exec( (boost::format("SELECT id, sections FROM %1%.road_restriction") % schema_name).str() ) );

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

            bool invalid = false;
            Road::Restriction::EdgeSequence edges;
            std::vector<db_id_t> sections = res[i][1].as<std::vector<db_id_t> >();
            for ( size_t j = 0; j < sections.size(); j++ ) {
                db_id_t id = sections[j];
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
        Db::Result res( connection_.exec( (boost::format("SELECT restriction_id, traffic_rules, time_value FROM %1%.road_restriction_time_penalty") % schema_name).str() ) );
        std::map<db_id_t, Road::Restriction::CostPerTransport> costs;
        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t restr_id;
            int transports;
            double cost = 0.0;

            res[i][0] >> restr_id;
            res[i][1] >> transports;
            res[i][2] >> cost;

            if ( edges_map.find( restr_id ) == edges_map.end() ) {
                CERR << "Cannot find road_restriction of ID " << restr_id << std::endl;
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
