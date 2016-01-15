#include "multimodal_graph_builder.hh"

#include <boost/format.hpp>
#include <iostream>
#include <fstream>

#include "serializers.hh"
#include "db.hh"
#include "multimodal_graph.hh"

namespace Tempus
{

using namespace std;

void import_constants( Db::Connection& connection, Multimodal::Graph& graph )
{
    // metadata
    load_metadata( graph, connection );

    // transport modes
    Multimodal::Graph::TransportModes modes = load_transport_modes( connection );
    graph.set_transport_modes( modes );
}

std::unique_ptr<Road::Graph> import_road_graph_( Db::Connection& connection, ProgressionCallback& /*progression*/, bool consistency_check, const std::string& schema_name, std::map<Tempus::db_id_t, Road::Edge>& road_sections_map )
{
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

            Db::Result res( connection.exec( q ) );
            size_t count = 0;
            res[0][0] >> count;
            if ( count ) {
                CERR << "[WARNING]: there are " << count << " road sections with inexistent node_from or node_to" << std::endl;
            }
        }
        {
            // look for cycles
            const std::string q = (boost::format("SELECT count(*) FROM %1%.road_section WHERE node_from = node_to") % schema_name).str();
            Db::Result res( connection.exec( q ) );
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

            Db::Result res( connection.exec( q ) );
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
            Db::Result res( connection.exec( q ) );
            size_t count = 0;
            res[0][0] >> count;
            if ( count ) {
                CERR << "[WARNING]: there are " << count << " unreferenced public transport stops" << std::endl;
            }
        }
    }

    // locally maps db ID to Node or Section
    std::map<Tempus::db_id_t, Road::Vertex> road_nodes_map;

    //------------------
    //   Road nodes
    //------------------
    std::vector<Road::Node> nodes;
    {
        Db::Result res = connection.exec( (boost::format("SELECT COUNT(*) FROM %1%.road_node") % schema_name).str() );
        size_t count = 0;
        res[0][0] >> count;
        nodes.reserve( count );
    }
    {
        Db::ResultIterator res_it = connection.exec_it( (boost::format("SELECT id, bifurcation, st_x(geom), st_y(geom), st_z(geom) FROM %1%.road_node") % schema_name).str() );
        Db::ResultIterator it_end;
        for ( ; res_it != it_end; res_it++ )
        {
            Db::RowValue res_i = *res_it;
            Road::Node node;

            node.set_db_id( res_i[0] );
            // only overwritten if not null
            node.set_is_bifurcation( res_i[1] );

            Point3D p;
            p.set_x( res_i[2] );
            p.set_y( res_i[3] );
            p.set_z( res_i[4] );
            node.set_coordinates(p);

            road_nodes_map[ node.db_id() ] = nodes.size();
            nodes.push_back( node );

            //progression( static_cast<float>( ( i + 0. ) / res.size() / 4.0 ) );
        }
    }

    //------------------
    //   Road sections
    //------------------
    std::map<std::pair<Road::Vertex, Road::Vertex>, Road::Section> sections;
    {
        //
        // Get a road section and its opposite, if present
        const std::string qquery = (boost::format("SELECT "
                                                  "rs1.id, rs1.road_type, rs1.node_from, rs1.node_to, rs1.traffic_rules_ft, "
                                                  "rs1.traffic_rules_tf, rs1.length, rs1.car_speed_limit, rs1.lane, "
                                                  "rs1.roundabout, rs1.bridge, rs1.tunnel, rs1.ramp, rs1.tollway, "
                                                  "rs2.id, rs2.road_type, "
                                                  "rs2.traffic_rules_ft, rs2.length, rs2.car_speed_limit, rs2.lane, "
                                                  "rs2.roundabout, rs2.bridge, rs2.tunnel, rs2.ramp, rs2.tollway "
                                                  "FROM %1%.road_section AS rs1 "
                                                  "LEFT JOIN %1%.road_section AS rs2 "
                                                  "ON rs1.node_from = rs2.node_to AND rs1.node_to = rs2.node_from "
                                                  // we add this constraint to avoid duplicates of ways such as (a,b) and (b,a)
                                                  // this assumes id are always positive numbers
                                                  "WHERE rs1.id > coalesce(rs2.id, -1)" 
                                                  ) % schema_name).str();
        Db::ResultIterator res_it = connection.exec_it( qquery );
        Db::ResultIterator it_end;
        for ( ; res_it != it_end; res_it++ )
        {
            Db::RowValue res_i = *res_it;
            Road::Section section;
            Road::Section section2;

            section.set_db_id( res_i[0].as<db_id_t>() );
            BOOST_ASSERT( section.db_id() > 0 );

            if ( !res_i[1].is_null() ) {
                section.set_road_type( static_cast<Road::RoadType>(res_i[1].as<int>()) );
            }

            db_id_t node_from_id = res_i[2].as<db_id_t>();
            db_id_t node_to_id = res_i[3].as<db_id_t>();
            BOOST_ASSERT( node_from_id > 0 );
            BOOST_ASSERT( node_to_id > 0 );

            int j = 4;
            int traffic_rules_ft = res_i[j++];
            int traffic_rules_tf = res_i[j++];
            section.set_traffic_rules( traffic_rules_ft );
            section.set_length( res_i[j++] );

            Db::Value car_speed_limit = res_i[j++];
            float car_speed_limit_f = car_speed_limit;
            if (car_speed_limit.is_null())
                car_speed_limit_f = 50;
            section.set_car_speed_limit( car_speed_limit_f );

            Db::Value lane = res_i[j++];
            if ( !lane.is_null() )
                section.set_lane( lane.as<int>() );
            else
                section.set_lane( 1 );
            section.set_is_roundabout( res_i[j++] );
            section.set_is_bridge( res_i[j++] );
            section.set_is_tunnel( res_i[j++] );
            section.set_is_ramp( res_i[j++] );
            section.set_is_tollway( res_i[j++] );

            if ( ! res_i[j].is_null() ) {
                //
                // If the opposite section exists, take it
                section2.set_db_id( res_i[j++].as<db_id_t>());
                section2.set_road_type( static_cast<Road::RoadType>(res_i[j++].as<int>()) );
                // overwrite transport_type_tf here
                traffic_rules_tf = res_i[j++];
                section2.set_length( res_i[j++] );

                Db::Value car_speed_limit_r = res_i[j++];
                car_speed_limit_f = car_speed_limit_r;
                if (car_speed_limit_r.is_null())
                    car_speed_limit_f = 50;
                section2.set_car_speed_limit( car_speed_limit_f );

                Db::Value lane2 = res_i[j++];
                if ( !lane2.is_null() )
                    section2.set_lane( lane2.as<int>() );
                else
                    section2.set_lane( 1 );
                section2.set_is_roundabout( res_i[j++] );
                section2.set_is_bridge( res_i[j++] );
                section2.set_is_tunnel( res_i[j++] );
                section2.set_is_ramp( res_i[j++] );
                section2.set_is_tollway( res_i[j++] );
            }
            else {
                // else create an opposite section from this one
                section2 = section;
            }
            section2.set_traffic_rules( traffic_rules_tf );

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
                // this edge should not exist
                // if it already exists, it means duplicate sections (multigraph) are present
                // and then ... ignored
                if ( sections.find( std::make_pair( v_from, v_to ) ) == sections.end() ) {
                    sections[std::make_pair( v_from, v_to )] = section;
                }
            }
            if ( traffic_rules_tf > 0 ) {
                // this edge should not exist
                if ( sections.find( std::make_pair( v_to, v_from ) ) == sections.end() ) {
                    sections[std::make_pair( v_to, v_from )] = section2;
                }
            }

            //progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.25 ) );
        }

        
    }

    std::cout << "CSR ..." << std::endl;
    auto l = [](decltype(sections)::value_type& p) { return p.first; };
    auto s_it_begin = boost::make_transform_iterator( sections.begin(), l );
    auto s_it_end = boost::make_transform_iterator( sections.end(), l );
    std::unique_ptr<Road::Graph> road_graph( new Road::Graph(boost::edges_are_unsorted_multi_pass,
                                                           s_it_begin, s_it_end,
                                                           nodes.size()) );

    std::cout << "Assigning nodes ..." << std::endl;
    {
        size_t i = 0;
        for ( const Road::Node& n : nodes ) {
            (*road_graph)[i] = n;
            i++;
        }
    }

    std::cout << "Assigning sections ..." << std::endl;
    for ( auto sit : sections ) {
        Road::Edge e;
        bool found;
        std::pair<Road::Vertex, Road::Vertex> p = sit.first;
        boost::tie(e, found) = edge( p.first, p.second, *road_graph );
        BOOST_ASSERT( found );
        // populate road_sections_map
        road_sections_map[ sit.second.db_id() ] = e;
        (*road_graph)[e] = sit.second;
    }

    return road_graph;
}

///
/// Function used to import the road and public transport graphs from a PostgreSQL database.
std::unique_ptr<Multimodal::Graph> import_graph( Db::Connection& connection, ProgressionCallback& progression, bool consistency_check, const std::string& schema_name )
{
    std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
    std::unique_ptr<Multimodal::Graph> graph;
    std::unique_ptr<Road::Graph> sm_road_graph( import_road_graph_( connection, progression, consistency_check, schema_name, road_sections_map ) );

    // build the multimodal graph
    graph.reset( new Multimodal::Graph( std::move(sm_road_graph) ) );
    Road::Graph* road_graph = &graph->road();

    // network_id -> pt_node_id -> vertex
    std::map<Tempus::db_id_t, std::map<Tempus::db_id_t, PublicTransport::Vertex> > pt_nodes_map;

    std::map<db_id_t, std::unique_ptr<PublicTransport::Graph>> pt_graphs;
    Multimodal::Graph::NetworkMap networks;
    {
        Db::Result res( connection.exec( "SELECT id, pnname FROM tempus.pt_network" ) );

        for ( size_t i = 0; i < res.size(); i++ ) {
            PublicTransport::Network network;

            network.set_db_id( res[i][0] );
            BOOST_ASSERT( network.db_id() > 0 );
            network.set_name( res[i][1] );

            networks[network.db_id()] = network;
            std::cout << "insert " << network.db_id() << std::endl;
            pt_graphs[network.db_id()].reset(new PublicTransport::Graph());
        }
    }

    {

        Db::ResultIterator res_it( connection.exec_it( (boost::format("select distinct on (n.id) "
                                                                    "s.network_id, n.id, n.name, n.location_type, "
                                                                    "n.parent_station, n.road_section_id, n.zone_id, n.abscissa_road_section "
                                                                    ",st_x(n.geom), st_y(n.geom), st_z(n.geom) "
                                                                    "from %1%.pt_stop as n, %1%.pt_section as s "
                                                                    "where s.stop_from = n.id or s.stop_to = n.id") % schema_name).str() ) );
        Db::ResultIterator it_end;
        for ( ; res_it != it_end; res_it++ ) {
            Db::RowValue res_i = *res_it;

            int j = 0;
            Tempus::db_id_t network_id;
            res_i[j++] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( networks.find( network_id ) != networks.end() );
            BOOST_ASSERT( pt_graphs.find( network_id ) != pt_graphs.end() );
            PublicTransport::Graph& pt_graph = *pt_graphs[network_id];
            PublicTransport::Stop stop;

            stop.set_db_id( res_i[j++] );
            BOOST_ASSERT( stop.db_id() > 0 );
            stop.set_name( res_i[j++] );
            stop.set_is_station( res_i[j++] );

            {
                Db::Value v(res_i[j++]);
                if ( ! v.is_null() ) {
                    Tempus::db_id_t p_id = v;
                    if ( pt_nodes_map[network_id].find( p_id ) != pt_nodes_map[network_id].end() ) {
                        stop.set_parent_station( pt_nodes_map[network_id][ p_id ] );
                    }
                }
            }

            Tempus::db_id_t road_section;
            res_i[j++] >> road_section;
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

            stop.set_zone_id( res_i[j++] );
            stop.set_abscissa_road_section( res_i[j++] );

            Point3D p;
            p.set_x( res_i[j++] );
            p.set_y( res_i[j++] );
            p.set_z( res_i[j++] );
            stop.set_coordinates(p);

            PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
            pt_nodes_map[network_id][ stop.db_id() ] = v;
            pt_graph[v].set_vertex( v );

            //progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.5 ) );
        }
    }

    //
    // For all public transport nodes, add a reference to the attached road section
    size_t gidx = 0;
    for ( auto it = pt_graphs.begin(); it != pt_graphs.end(); it++, gidx++ ) {
        PublicTransport::Graph& g = *it->second;
        PublicTransport::VertexIterator vi, vi_end;

        for ( boost::tie( vi, vi_end ) = boost::vertices( g ); vi != vi_end; vi++ ) {
            Road::Edge rs = g[ *vi ].road_edge();
            graph->add_stop_ref( rs, gidx, *vi );
            // add a ref to the opposite road edge, if any
            if (g[*vi].opposite_road_edge()) {
                rs = g[*vi].opposite_road_edge().get();
                graph->add_stop_ref( rs, gidx, *vi );
            }
        }
    }

    {
        Db::ResultIterator res_it( connection.exec_it( (boost::format("SELECT network_id, stop_from, stop_to FROM %1%.pt_section ORDER BY network_id") % schema_name).str() ) );
        Db::ResultIterator it_end;
        for ( ; res_it != it_end; res_it++ ) {
            Db::RowValue res_i = *res_it;
            Tempus::db_id_t network_id;
            res_i[0] >> network_id;
            BOOST_ASSERT( network_id > 0 );
            BOOST_ASSERT( pt_graphs.find( network_id ) != pt_graphs.end() );
            PublicTransport::Graph& pt_graph = *pt_graphs[ network_id ];

            Tempus::db_id_t stop_from_id, stop_to_id;
            res_i[1] >> stop_from_id;
            res_i[2] >> stop_to_id;
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
            pt_graph[e].set_network_id( network_id );

            //progression( static_cast<float>( ( ( i + 0. ) / res.size() / 4.0 ) + 0.75 ) );
        }

        // assign graph index to stops
        size_t graph_idx = 0;
        for ( auto p : pt_nodes_map ) {
            for ( auto p2 : p.second ) {
                (*pt_graphs[p.first])[p2.second].set_graph( graph_idx );
            }
            graph_idx++;
        }
    }

    //-------------
    //    POI
    //-------------
    std::vector<POI> pois;
    {
        Db::Result res = connection.exec( (boost::format("SELECT COUNT(*) FROM %1%.poi") % schema_name).str() );
        size_t count = 0;
        res[0][0] >> count;
        pois.reserve( count );
    }
    {
        Db::ResultIterator res_it( connection.exec_it( (boost::format("SELECT id, poi_type, name, parking_transport_modes, road_section_id, abscissa_road_section "
                                                                    ", st_x(geom), st_y(geom), st_z(geom) FROM %1%.poi") % schema_name).str() ) );
        Db::ResultIterator it_end;
        size_t pidx = 0;
        for ( ; res_it != it_end; res_it++, pidx++ ) {
            Db::RowValue res_i = *res_it;
            POI poi;

            poi.set_db_id( res_i[0] );
            BOOST_ASSERT( poi.db_id() > 0 );

            poi.set_poi_type( static_cast<POI::PoiType>(res_i[1].as<int>()) );
            poi.set_name( res_i[2] );

            poi.set_parking_transport_modes( res_i[3].as< std::vector<db_id_t> >() );

            db_id_t road_section_id;
            res_i[4] >> road_section_id;
            BOOST_ASSERT( road_section_id > 0 );

            if ( road_sections_map.find( road_section_id ) == road_sections_map.end() ) {
                CERR << "Cannot find road_section of ID " << road_section_id << ", poi " << poi.db_id() << " rejected" << endl;
                continue;
            }

            Road::Edge re = road_sections_map[ road_section_id ];
            poi.set_road_edge( re );
            graph->add_poi_ref( re, pidx );
            // look for an opposite road edge
            {
                Road::Edge opposite_edge;
                bool found;
                boost::tie( opposite_edge, found ) = edge( target( poi.road_edge(), *road_graph ),
                                                           source( poi.road_edge(), *road_graph ),
                                                           *road_graph );
                if ( found && (graph->road()[poi.road_edge()].db_id() == graph->road()[opposite_edge].db_id()) ) {
                    poi.set_opposite_road_edge( opposite_edge );
                    graph->add_poi_ref( opposite_edge, pidx );
                }
            }

            poi.set_abscissa_road_section( res_i[5] );

            Point3D p;
            p.set_x( res_i[6] );
            p.set_y( res_i[7] );
            p.set_z( res_i[8] );
            poi.set_coordinates( p );

            pois.push_back( poi );
        }
    }

    /// assign to the multimodal graph
    graph->set_network_map( networks );
    graph->set_public_transports( std::move(pt_graphs) );
    graph->set_pois( std::move(pois) );

    progression( 1.0, /* finished = */ true );

    return graph;
}


///
/// Import turn restrictions
Road::Restrictions import_turn_restrictions( Db::Connection& connection, const Road::Graph& graph, const std::string& schema_name )
{
    Road::Restrictions restrictions( graph );

    // temp restriction storage
    typedef std::map<db_id_t, Road::Restriction::EdgeSequence> EdgesMap;
    EdgesMap edges_map;

    {
        Db::Result res( connection.exec( (boost::format("SELECT id, sections FROM %1%.road_restriction") % schema_name).str() ) );

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
        Db::Result res( connection.exec( (boost::format("SELECT restriction_id, traffic_rules, time_value FROM %1%.road_restriction_time_penalty") % schema_name).str() ) );
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


std::unique_ptr<RoutingData> MultimodalGraphBuilder::pg_import( const std::string& pg_options, ProgressionCallback& progression, const VariantMap& options ) const
{
    Db::Connection connection( pg_options );

    bool consistency_check = true;
    auto consistency_it = options.find( "consistency_check" );
    if ( consistency_it != options.end() ) {
        consistency_check = consistency_it->second.as<bool>();
    }

    std::string schema_name = "tempus";
    auto schema_it = options.find( "schema_name" );
    if ( schema_it != options.end() ) {
        schema_name = schema_it->second.str();
    }

    std::unique_ptr<Multimodal::Graph> mm_graph( import_graph( connection, progression, consistency_check, schema_name ) );
    import_constants( connection, *mm_graph );
    std::unique_ptr<RoutingData> rgraph( mm_graph.release() );
    return std::move(rgraph);
}

std::unique_ptr<RoutingData> MultimodalGraphBuilder::file_import( const std::string& filename, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
    std::unique_ptr<Road::Graph> rgraph;
    std::unique_ptr<Multimodal::Graph> graph( new Multimodal::Graph(std::move(rgraph)) );
    std::ifstream ifs( filename );
    if ( ifs.fail() ) {
        throw std::runtime_error( "Problem opening input file " + filename );
    }

    read_header( ifs );

    unserialize( ifs, *graph, binary_serialization_t() );
    std::unique_ptr<RoutingData> rd( graph.release() );
    return std::move( rd );
}

void MultimodalGraphBuilder::file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
    std::ofstream ofs( filename );

    write_header( ofs );

    const Multimodal::Graph* graph = static_cast<const Multimodal::Graph*>( rd );
    serialize( ofs, *graph, binary_serialization_t() );
}

REGISTER_BUILDER( MultimodalGraphBuilder )

}
