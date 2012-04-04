/*
  Unit test architecture

  Covers:
  * pgsql_importer : check import functions, consistency, robustness, performances ?
  * plugin : load / unload

  */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../db.hh"

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
    CPPUNIT_TEST( testRobustness );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testConsistency();
    void testRobustness();
protected:
    Db::Connection* connection_;
};
