/*
  Unit test architecture

  Covers:
  * pgsql_importer : check import functions, consistency, robustness, performances ?
  * plugin : load / unload

  */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "db.hh"
#include "pgsql_importer.hh"
#include "multimodal_graph.hh"

extern std::string g_db_options;

class DbTest : public CppUnit::TestFixture  {

    CPPUNIT_TEST_SUITE( DbTest );
    CPPUNIT_TEST( testConnection );
    CPPUNIT_TEST( testQueries );
    CPPUNIT_TEST_SUITE_END();

public:
    void testConnection();
    void testQueries();

protected:
    Db::Connection* connection_;
};

class PgImporterTest : public CppUnit::TestFixture  {

    CPPUNIT_TEST_SUITE( PgImporterTest );
    CPPUNIT_TEST( testConsistency );
    CPPUNIT_TEST( testMultimodal );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testConsistency();
    void testMultimodal();
protected:
    Tempus::PQImporter *  importer_;

    Tempus::Multimodal::Graph graph_;
};
