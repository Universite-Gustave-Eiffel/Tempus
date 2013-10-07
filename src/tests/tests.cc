#include <iostream>
#include <string>

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "tests.hh"

using namespace std;
using namespace Tempus;

#define DB_TEST_NAME "tempus_test_db"

void DbTest::testConnection()
{
    COUT << "DbTest::testConnection()" << endl;
    string db_options = g_db_options;

    // Connection to an non-existing database
    bool has_thrown = false;
    try
    {
	connection_ = new Db::Connection( db_options + " dbname=zorglub" );
    }
    catch ( std::runtime_error& )
    {
	has_thrown = true;
    }
    CPPUNIT_ASSERT_MESSAGE( "Must throw an exception when trying to connect to a non existing db", has_thrown );

    // Connection to an existing database
    has_thrown = false;
    try
    {
	connection_ = new Db::Connection( db_options + " dbname = " DB_TEST_NAME );
    }
    catch ( std::runtime_error& )
    {
	has_thrown = true;
    }
    CPPUNIT_ASSERT_MESSAGE( "Must not throw on an existing database, check that " DB_TEST_NAME " exists", !has_thrown );

    // Do not sigsegv ?
    delete connection_;
}

void DbTest::testQueries()
{
    COUT << "DbTest::testQueries()" << endl;
    string db_options = g_db_options;
    connection_ = new Db::Connection( db_options + " dbname = " DB_TEST_NAME );

    // test bad query
    bool has_thrown = false;
    try
    {
	connection_->exec( "SELZECT * PHROM zorglub" );
    }
    catch ( std::runtime_error& )
    {
	has_thrown = true;
    }
    CPPUNIT_ASSERT_MESSAGE( "Must throw an exception on bad SQL query", has_thrown );

    connection_->exec( "DROP TABLE IF EXISTS test_table" );
    connection_->exec( "CREATE TABLE test_table (id int, int_v int, bigint_v bigint, str_v varchar, time_v time)" );
    connection_->exec( "INSERT INTO test_table (id, int_v) VALUES ('1', '42')" );
    connection_->exec( "INSERT INTO test_table (id, int_v, bigint_v) VALUES ('2', '-42', '10000000000')" );
    connection_->exec( "INSERT INTO test_table (str_v) VALUES ('Hello world')" );
    connection_->exec( "INSERT INTO test_table (time_v) VALUES ('13:52:45')" );
    Db::Result res = connection_->exec( "SELECT * FROM test_table" );
    
    CPPUNIT_ASSERT_EQUAL( (size_t)4, res.size() );
    CPPUNIT_ASSERT_EQUAL( (size_t)5, res.columns() );
    CPPUNIT_ASSERT_EQUAL( (int)1, res[0][0].as<int>() );
    CPPUNIT_ASSERT_EQUAL( (int)42, res[0][1].as<int>() );
    CPPUNIT_ASSERT( res[0][2].is_null() );

    CPPUNIT_ASSERT_EQUAL( (int)-42, res[1][1].as<int>());
    CPPUNIT_ASSERT_EQUAL( 10000000000ULL, res[1][2].as<unsigned long long>() );

    CPPUNIT_ASSERT_EQUAL( string("Hello world"), res[2][3].as<string>() );

    Tempus::Time t = res[3][4].as<Tempus::Time>();
    CPPUNIT_ASSERT_EQUAL( (long)(13 * 3600 + 52 * 60 + 45), t.n_secs );

    delete connection_;
}

void PgImporterTest::setUp()
{
    string db_options = g_db_options;
    importer_ = new PQImporter( db_options + " dbname = " DB_TEST_NAME );
}

void PgImporterTest::tearDown() 
{
    delete importer_;
}

void PgImporterTest::testConsistency()
{
    COUT << "PgImporterTest::testConsistency()" << endl;
    importer_->import_constants( graph_ );
    importer_->import_graph( graph_ );

    // get the number of vertices in the graph
    Db::Result res = importer_->query( "SELECT COUNT(*) FROM tempus.road_node" );
    CPPUNIT_ASSERT( res.size() == 1 );
    long n_road_vertices = res[0][0].as<long>();
    res = importer_->query( "SELECT COUNT(*) FROM tempus.road_section" );
    CPPUNIT_ASSERT( res.size() == 1 );
    long n_road_edges = res[0][0].as<long>();
    COUT << "n_road_vertices = " << n_road_vertices << " n_road_edges = " << n_road_edges << endl;
    CPPUNIT_ASSERT( n_road_vertices = boost::num_vertices( graph_.road ) );
    CPPUNIT_ASSERT( n_road_edges = boost::num_edges( graph_.road ) );
    
    // number of PT networks
    res = importer_->query( "SELECT COUNT(*) FROM tempus.pt_network" );
    long n_networks = res[0][0].as<long>();
	
    CPPUNIT_ASSERT_EQUAL( (size_t)n_networks, graph_.public_transports.size() );
    CPPUNIT_ASSERT_EQUAL( (size_t)n_networks, graph_.network_map.size() );

    Multimodal::Graph::PublicTransportGraphList::iterator it;
    for ( it = graph_.public_transports.begin(); it != graph_.public_transports.end(); it++ )
    {
	db_id_t id = it->first;
	PublicTransport::Graph& pt_graph = it->second;

	res = importer_->query( "SELECT COUNT(*) FROM tempus.pt_stop" );
	CPPUNIT_ASSERT( res.size() == 1 );
	long n_pt_vertices = res[0][0].as<long>();
	res = importer_->query( "SELECT COUNT(*) FROM tempus.pt_section" );
	CPPUNIT_ASSERT( res.size() == 1 );
	long n_pt_edges = res[0][0].as<long>();
	COUT << "n_pt_vertices = " << n_pt_vertices << " num_vertices(pt_graph) = " << num_vertices(pt_graph) << std::endl;
	CPPUNIT_ASSERT( n_pt_vertices = boost::num_vertices( pt_graph ) );
	COUT << "n_pt_edges = " << n_pt_edges << " num_edges(pt_graph) = " << num_edges(pt_graph) << std::endl;
	CPPUNIT_ASSERT( n_pt_edges = boost::num_edges( pt_graph ) );
    }
}

Multimodal::Vertex vertex_from_road_node_id( db_id_t id, const Multimodal::Graph& graph_ )
{
    Multimodal::VertexIterator vi, vi_end;
    for ( boost::tie( vi, vi_end ) = vertices( graph_ ); vi != vi_end; vi++ )
    {
	if ( vi->type == Multimodal::Vertex::Road && graph_.road[ vi->road_vertex ].db_id == id )
	{
	    return *vi;
	}
    }
}


void PgImporterTest::testMultimodal()
{
    COUT << "PgImporterTest::testMultimodal()" << endl;
    importer_->import_constants( graph_ );
    importer_->import_graph( graph_ );

    size_t nv = 0;
    size_t n_road_vertices = 0;
    size_t n_pt_vertices = 0;
    size_t n_pois = 0;
    Multimodal::VertexIterator vi, vi_end;
    for ( boost::tie(vi, vi_end) = vertices( graph_ ); vi != vi_end; vi++ )
    {
	nv++;
	if ( vi->type == Multimodal::Vertex::Road )
	    n_road_vertices++;
	else if ( vi->type == Multimodal::Vertex::PublicTransport )
	    n_pt_vertices++;
	else
	    n_pois ++;
    }

    const PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
    COUT << "nv = " << nv << endl;
    COUT << "n_road_vertices = " << n_road_vertices << " num_vertices(road) = " << num_vertices( graph_.road ) << endl;
    COUT << "n_pt_vertices = " << n_pt_vertices << " num_vertices(pt) = " << num_vertices( pt_graph ) << endl;
    COUT << "n_pois = " << n_pois << " pois.size() = " << graph_.pois.size() << endl;
    COUT << "num_vertices = " << num_vertices( graph_ ) << endl;
    CPPUNIT_ASSERT_EQUAL( nv, num_vertices( graph_ ) );

    for ( boost::tie(vi, vi_end) = vertices( graph_ ); vi != vi_end; vi++ )
    {
    	Multimodal::OutEdgeIterator oei, oei_end;
    	boost::tie( oei, oei_end ) = out_edges( *vi, graph_ );
	size_t out_deg = 0;
    	for ( ; oei != oei_end; oei++ )
    	{
    	    out_deg++;
    	}
	size_t out_deg2 = out_degree( *vi, graph_ );
	CPPUNIT_ASSERT_EQUAL( out_deg, out_deg2 );
    }
    Multimodal::EdgeIterator ei, ei_end;
    size_t ne = 0;
    size_t n_road2road = 0;
    size_t n_road2transport = 0;
    size_t n_transport2road = 0;
    size_t n_transport2transport = 0;
    size_t n_road2poi = 0;
    size_t n_poi2road = 0;
    
    Road::OutEdgeIterator ri, ri_end;
    Road::Vertex v1 = *(vertices( graph_.road ).first);
    boost::tie( ri, ri_end ) = out_edges( v1, graph_.road );
    //    BOOST_ASSERT( ri != ri_end );

    for ( boost::tie( ei, ei_end ) = edges( graph_ ); ei != ei_end; ei++ )
    {
	ne++;
	switch ( ei->connection_type() )
	{
	case Multimodal::Edge::Road2Road:
	    n_road2road++;
	    break;
	case Multimodal::Edge::Road2Transport:
	    n_road2transport++;
	    break;
	case Multimodal::Edge::Transport2Road:
	    n_transport2road++;
	    break;
	case Multimodal::Edge::Transport2Transport:
	    n_transport2transport++;
	    break;
	case Multimodal::Edge::Road2Poi:
	    n_road2poi++;
	    break;
	case Multimodal::Edge::Poi2Road:
	    n_poi2road++;
	    break;
	}
    }

    size_t n_stops = 0;
    Road::EdgeIterator pei, pei_end;
    for ( boost::tie( pei, pei_end ) = edges( graph_.road ); pei != pei_end; pei++ )
    {
	n_stops += graph_.road[ *pei ].stops.size();
    }

    COUT << "ne = " << ne << endl;
    COUT << "n_road2road = " << n_road2road << " num_edges(road) = " << num_edges( graph_.road ) << endl;
    COUT << "n_road2transport = " << n_road2transport << endl;
    COUT << "n_transport2road = " << n_transport2road << endl;
    COUT << "n_road2poi = " << n_road2poi << endl;
    COUT << "n_poi2road = " << n_poi2road << " pois.size = " << graph_.pois.size() << endl;
    COUT << "n_transport2transport = " << n_transport2transport << " num_edges(pt) = " << num_edges( pt_graph ) << endl;
    size_t sum = n_road2road + n_road2transport + n_transport2road + n_transport2transport + n_poi2road + n_road2poi;
    COUT << "sum = " << sum << endl;
    COUT << "num_edges = " << num_edges( graph_ ) << endl;
    CPPUNIT_ASSERT_EQUAL( sum, num_edges( graph_ ) );

    // test vertex index
    Multimodal::VertexIndexProperty index = get( boost::vertex_index, graph_ );
    for ( boost::tie(vi, vi_end) = vertices( graph_ ); vi != vi_end; vi++ )
    {
	size_t idx = get( index, *vi );
	if ( vi->type == Multimodal::Vertex::Road )
	{
	    CPPUNIT_ASSERT( idx < num_vertices( graph_.road ) );
	}
    }
 
    // test that graph vertices and edges can be used as a map key
    // i.e. the operator< forms a complete order
    {
	    std::set< Multimodal::Vertex > vertex_set;
	    for ( boost::tie(vi, vi_end) = vertices( graph_ ); vi != vi_end; ++vi ) {
		    vertex_set.insert( *vi );
	    }
	    // check whether we have one entry per vertex
	    CPPUNIT_ASSERT_EQUAL( vertex_set.size(), num_vertices( graph_ ));

	    std::set< Multimodal::Edge > edge_set;
	    for ( boost::tie(ei, ei_end) = edges( graph_ ); ei != ei_end; ++ei ) {
		    edge_set.insert( *ei );
	    }
	    // check whether we have one entry per vertex
	    CPPUNIT_ASSERT_EQUAL( edge_set.size(), num_edges( graph_ ));
    }

    // test graph traversal
    {
	std::map<Multimodal::Vertex, boost::default_color_type> colors;
	boost::depth_first_search( graph_,
				   boost::dfs_visitor<boost::null_visitor>(),
				   boost::make_assoc_property_map( colors )
				   );
    }

    // test dijkstra
    {
	size_t n = num_vertices( graph_ );
	std::vector<boost::default_color_type> color_map( n );
	std::vector<Multimodal::Vertex> pred_map( n );
	std::vector<double> distance_map( n );
	std::map< Multimodal::Edge, double > lengths;
	
	Multimodal::EdgeIterator ei, ei_end;
	for ( boost::tie( ei, ei_end ) = edges( graph_ ); ei != ei_end; ei++ )
	{
	    if ( ei->connection_type() == Multimodal::Edge::Road2Road )
	    {
		lengths[ *ei ] = 10.0;
	    }
	    else
	    {
		lengths[ *ei ] = 1.0;
	    }
	}
	
	Multimodal::Vertex origin, destination;
	origin = vertex_from_road_node_id( 19953, graph_ );
	destination = vertex_from_road_node_id( 22510, graph_ );
	
	COUT << "origin = " << origin << endl;
	COUT << "destination = " << destination << endl;
	
	Multimodal::VertexIndexProperty vertex_index = get( boost::vertex_index, graph_ );
	COUT << "ok" << endl;
	
	boost::dijkstra_shortest_paths( graph_,
					origin,
					boost::make_iterator_property_map( pred_map.begin(), vertex_index ),
					boost::make_iterator_property_map( distance_map.begin(), vertex_index ),
					boost::make_assoc_property_map( lengths ),
					vertex_index,
					std::less<double>(),
					boost::closed_plus<double>(),
					std::numeric_limits<double>::max(),
					0.0,
					boost::dijkstra_visitor<boost::null_visitor>(),
					boost::make_iterator_property_map( color_map.begin(), vertex_index )
					);
	COUT << "Dijkstra OK" << endl;
    }

    // test public transport sub map
    {
	// 1 // create other public transport networks, if needed
	if ( graph_.public_transports.size() < 2 )
	{
	    // get the maximum pt id
	    db_id_t max_id = 0;
	    for ( Multimodal::Graph::PublicTransportGraphList::const_iterator it = graph_.public_transports.begin();
		  it != graph_.public_transports.end();
		  it++ )
	    {
		if ( it->first > max_id )
		{
		    max_id = it->first;
		}
	    }
	    size_t n_vertices = num_vertices( graph_ );
	    size_t n_edges = num_edges( graph_ );

	    // insert a new pt that is a copy of the first
	    graph_.public_transports[max_id+1] = graph_.public_transports.begin()->second;
	    graph_.public_transports.select_all();

	    size_t n_vertices2 = num_vertices( graph_ );
	    size_t n_edges2 = num_edges( graph_ );

	    // unselect the first network
	    std::set<db_id_t> selection = graph_.public_transports.selection();
	    selection.erase( graph_.public_transports.begin()->first );
	    graph_.public_transports.select( selection );

	    size_t n_vertices3 = num_vertices( graph_ );
	    size_t n_edges3 = num_edges( graph_ );
	    size_t n_computed_vertices = 0;
	    Multimodal::VertexIterator vi, vi_end;
	    for ( boost::tie( vi, vi_end ) = vertices( graph_ ); vi != vi_end; vi++ )
	    {
		n_computed_vertices ++;
	    }

	    size_t n_computed_edges = 0;
	    Multimodal::EdgeIterator ei, ei_end;
	    for ( boost::tie( ei, ei_end ) = edges( graph_ ); ei != ei_end; ei++ )
	    {
		n_computed_edges ++;
	    }

	    // check that number of vertices are still ok
	    CPPUNIT_ASSERT_EQUAL( n_vertices, n_vertices3 );
	    // check the use of vertex iterators
	    CPPUNIT_ASSERT_EQUAL( n_computed_vertices, n_vertices3 );
	    // check that number of vertices are still ok
	    CPPUNIT_ASSERT_EQUAL( n_edges, n_edges3 );
	    // check the use of edges iterators
	    CPPUNIT_ASSERT_EQUAL( n_computed_edges, n_edges3 );
	}
    }
}

