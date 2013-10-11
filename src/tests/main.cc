#define BOOST_TEST_MODULE UnitTestTempus
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test ;

test_suite* init_unit_test_suite( int, char * * const )
{
    return 0;
}

