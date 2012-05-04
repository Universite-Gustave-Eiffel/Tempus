#include <iostream>
#include <string>

#include "tests.hh"

using namespace std;
using namespace Tempus;

#define DB_TEST_NAME "tempus_test_db"

void DbTest::testConnection()
{
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
    importer_->import_constants( graph_ );
    importer_->import_graph( graph_ );

    // get the number of vertices in the graph
    Db::Result res = importer_->query( "SELECT COUNT(*) FROM tempus.road_node" );
    CPPUNIT_ASSERT( res.size() == 1 );
    long n_road_vertices = res[0][0].as<long>();
    res = importer_->query( "SELECT COUNT(*) FROM tempus.road_section" );
    CPPUNIT_ASSERT( res.size() == 1 );
    long n_road_edges = res[0][0].as<long>();
    CPPUNIT_ASSERT( n_road_vertices = boost::num_vertices( graph_.road ) );
    CPPUNIT_ASSERT( n_road_edges = boost::num_edges( graph_.road ) );
    
    // number of PT networks
    res = importer_->query( "SELECT COUNT(*) FROM tempus.pt_network" );
    long n_networks = res[0][0].as<long>();
	
    CPPUNIT_ASSERT_EQUAL( (size_t)n_networks, graph_.public_transports.size() );
    CPPUNIT_ASSERT_EQUAL( (size_t)n_networks, graph_.network_map.size() );

    MultimodalGraph::PublicTransportGraphList::iterator it;
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
	CPPUNIT_ASSERT( n_pt_vertices = boost::num_vertices( pt_graph ) );
	CPPUNIT_ASSERT( n_pt_edges = boost::num_edges( pt_graph ) );
    }
}


