#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/CompilerOutputter.h>

#include "tests.hh"

CPPUNIT_TEST_SUITE_REGISTRATION( DbTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PgImporterTest );

std::string g_db_options;

int main( int argc, char **argv )
{
	if ( argc > 1 )
	{
		g_db_options=argv[1];
	}
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest( registry.makeTest() );
    runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(),
							 std::cerr ) );
    return runner.run( "", false ) ? EXIT_SUCCESS : EXIT_FAILURE;   
}
