#include <iostream>
#include <string>

#include "tests.hh"

using namespace std;

#define DB_COMMON_OPTIONS ""
#define DB_TEST_NAME "test_db"

void DbTest::testConnection()
{
    string db_options = DB_COMMON_OPTIONS;

    // Connection to an non-existing database
    bool has_thrown = false;
    try
    {
	connection_ = new Db::Connection( db_options + " dbname=zorglub" );
    }
    catch ( std::runtime_error& e )
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
    catch ( std::runtime_error& e )
    {
	has_thrown = true;
    }
    CPPUNIT_ASSERT_MESSAGE( "Must not throw on an existing database, check that " DB_TEST_NAME " exists", !has_thrown );

    // Do not sigsegv ?
    delete connection_;
}

void DbTest::testQueries()
{
    string db_options = DB_COMMON_OPTIONS;
    connection_ = new Db::Connection( db_options + " dbname = " DB_TEST_NAME );

    // test bad query
    bool has_thrown = false;
    try
    {
	connection_->exec( "SELZECT * PHROM zorglub" );
    }
    catch ( std::runtime_error& e )
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
    
    CPPUNIT_ASSERT( res.size() == 4 );
    CPPUNIT_ASSERT( res.columns() == 5 );
    CPPUNIT_ASSERT( res[0][0].as<int>() == 1 );
    CPPUNIT_ASSERT( res[0][1].as<int>() == 42 );
    CPPUNIT_ASSERT( res[0][2].is_null() );

    CPPUNIT_ASSERT( res[1][1].as<int>() == -42 );
    CPPUNIT_ASSERT( res[1][2].as<long long>() == 10000000000LL );

    CPPUNIT_ASSERT( res[2][3].as<string>() == "Hello world" );

    Tempus::Time t = res[3][4].as<Tempus::Time>();
    CPPUNIT_ASSERT( t.n_secs = 13 * 3600 + 52 * 60 + 45 );

    delete connection_;
}

void PgImporterTest::setUp()
{
}

void PgImporterTest::tearDown() 
{
}

void PgImporterTest::testConsistency()
{
}

void PgImporterTest::testRobustness()
{
}

