#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/CompilerOutputter.h>

#include "tests.hh"

CPPUNIT_TEST_SUITE_REGISTRATION( DbTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PgImporterTest );

int main( int argc, char **argv )
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest( registry.makeTest() );
    runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(),
							 std::cerr ) );
    bool wasSuccessful = runner.run( "", false );
    return wasSuccessful;   
}
