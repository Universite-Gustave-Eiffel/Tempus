#include <libpq-fe.h>

#include <iostream>

#include <boost/mpl/vector.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "pgsql_importer.hh"

using namespace std;

namespace Tempus
{
    PQImporter::PQImporter( const std::string& pg_options ) :
	connection_( pg_options )
    {
    }
    
    Db::Result PQImporter::query( const std::string& query_str )
    {
	return connection_.exec( query_str );
    }

    void PQImporter::import_constants( ProgressionCallback& progression )
    {
	Db::Result res = connection_.exec( "SELECT id, parent_id, ttname, need_parking, need_station, need_return FROM tempus.transport_type" );
	for ( size_t i = 0; i < res.size(); i++ )
	{
	    db_id_t db_id;
	    res[i][0] >> db_id;
	    BOOST_ASSERT( db_id > 0 );
	    // populate the global variable
	    TransportType& t = Tempus::transport_types[ db_id ];
	    t.db_id = db_id;
	    // default parent_id == null
	    t.parent_id = 0;
	    res[i][1] >> t.parent_id; // <- if the field is null, t.parent_id is kept untouched
	    res[i][2] >> t.name;
	    res[i][3] >> t.need_parking;
	    res[i][4] >> t.need_station;
	    res[i][5] >> t.need_return;

	    // assign the name <-> id map
	    Tempus::transport_type_from_name[ t.name ] = t.db_id;
	    
	    progression( static_cast<float>((i + 0.) / res.size() / 2.0) );
	}

	res = connection_.exec( "SELECT id, rtname FROM tempus.road_type" );
	for ( size_t i = 0; i < res.size(); i++ )
	{
	    db_id_t db_id;
	    res[i][0] >> db_id;
	    BOOST_ASSERT( db_id > 0 );
	    // populate the global variable
	    RoadType& t = Tempus::road_types[ db_id ];
	    t.db_id = db_id;
	    res[i][1] >> t.name;
	    // assign the name <-> id map
	    Tempus::road_type_from_name[ t.name ] = t.db_id;
	    
	    progression( static_cast<float>(((i + 0.) / res.size() / 2.0) + 0.5));
	}
    }

    ///
    /// Function used to import the road and public transport graphs from a PostgreSQL database.
    void PQImporter::import_graph( MultimodalGraph& graph, ProgressionCallback& progression )
    {
	Road::Graph& road_graph = graph.road;

	// locally maps db ID to Node or Section
	std::map<Tempus::db_id_t, Road::Vertex> road_nodes_map;
	std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
	// network_id -> pt_node_id -> vertex
	std::map<Tempus::db_id_t, std::map<Tempus::db_id_t, PublicTransport::Vertex> > pt_nodes_map;

	Db::Result res = connection_.exec( "SELECT id, junction, bifurcation FROM tempus.road_node" );
	
	for ( size_t i = 0; i < res.size(); i++ )
	{
	    Road::Node node;
	    
	    node.db_id = res[i][0].as<db_id_t>();
	    node.is_junction = false;
	    node.is_bifurcation = false;
	    BOOST_ASSERT( node.db_id > 0 );
	    // only overwritten if not null
	    res[i][1] >> node.is_junction;
	    res[i][2] >> node.is_bifurcation;
	    
	    Road::Vertex v = boost::add_vertex( node, road_graph );
	    
	    road_nodes_map[ node.db_id ] = v;
	    road_graph[v].vertex = v;
	    
	    progression( static_cast<float>((i + 0.) / res.size() / 4.0) );
	}
	
	std::string query = "SELECT id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, "
	    "car_average_speed, road_name, lane, "
	    "roundabout, bridge, tunnel, ramp, tollway FROM tempus.road_section";
	res = connection_.exec( query );
	
	for ( size_t i = 0; i < res.size(); i++ )
	{
	    Road::Section section;
	    
	    res[i][0] >> section.db_id;
	    BOOST_ASSERT( section.db_id > 0 );
	    if ( !res[i][1].is_null() )
		section.road_type = res[i][1].as<db_id_t>();
	    
	    db_id_t node_from_id = res[i][2].as<db_id_t>();
	    db_id_t node_to_id = res[i][3].as<db_id_t>();
	    
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
			      (boost::format("Non existing node_from %1% on road_section %2%") % node_from_id % section.db_id).str().c_str());
	    BOOST_ASSERT_MSG( road_nodes_map.find( node_to_id ) != road_nodes_map.end(),
			      (boost::format("Non existing node_to %1% on road_section %2%") % node_to_id % section.db_id).str().c_str());
	    Road::Vertex v_from = road_nodes_map[ node_from_id ];
	    Road::Vertex v_to = road_nodes_map[ node_to_id ];

	    Road::Edge e;
	    bool is_added, found;
	    boost::tie( e, found ) = boost::edge( v_from, v_to, road_graph );
	    if ( found )
	    {
		cout << "Edge " << e << " already exists" << endl;
		continue;
	    }
	    boost::tie( e, is_added ) = boost::add_edge( v_from, v_to, section, road_graph );
	    BOOST_ASSERT( is_added );
	    road_graph[e].edge = e;
	    road_sections_map[ section.db_id ] = e;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.25) );
	}

	res = connection_.exec( "SELECT id, pnname FROM tempus.pt_network" );
	for ( size_t i = 0; i < res.size(); i++ )
	{
	    PublicTransport::Network network;
	    
	    res[i][0] >> network.db_id;
	    BOOST_ASSERT( network.db_id > 0 );
	    res[i][1] >> network.name;

	    graph.network_map[network.db_id] = network;
	    graph.public_transports[network.db_id] = PublicTransport::Graph();
	}

	res = connection_.exec( "SELECT DISTINCT s.network_id, n.id, n.psname, n.location_type, n.parent_station, n.road_section_id, n.zone_id, n.abscissa_road_section FROM tempus.pt_stop as n JOIN tempus.pt_section as s ON s.stop_from = n.id OR s.stop_to = n.id" );
	
	for ( size_t i = 0; i < res.size(); i++ )
	{
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
	    int parent_station;
	    res[i][j++] >> parent_station;
	    if ( pt_nodes_map[network_id].find( parent_station ) != pt_nodes_map[network_id].end() )
	    {
		stop.parent_station = pt_nodes_map[network_id][ parent_station ];
		stop.has_parent = true;
	    }

	    int road_section;
	    res[i][j++] >> road_section;
	    stop.road_section = road_sections_map[ road_section ];

	    res[i][j++] >> stop.zone_id;
	    res[i][j++] >> stop.abscissa_road_section;

	    PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
	    pt_nodes_map[network_id][ stop.db_id ] = v;
	    pt_graph[v].vertex = v;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.5) );
	}

	res = connection_.exec( "SELECT network_id, stop_from, stop_to FROM tempus.pt_section" );
     
	for ( size_t i = 0; i < res.size(); i++ )
	{
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

	    PublicTransport::Vertex stop_from = pt_nodes_map[network_id][ stop_from_id ];
	    PublicTransport::Vertex stop_to = pt_nodes_map[network_id][ stop_to_id ];
	    PublicTransport::Edge e;
	    bool is_added;
	    boost::tie( e, is_added ) = boost::add_edge( stop_from, stop_to, pt_graph );
	    BOOST_ASSERT( is_added );
	    pt_graph[e].edge = e;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.75) );
	}

	progression( 1.0, /* finished = */ true );

    }

};
