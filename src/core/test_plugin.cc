#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"

using namespace std;

///
/// TestPlugin used for unit tests

namespace Tempus
{

    class TestPlugin : public Plugin
    {
    public:

	TestPlugin( Db::Connection& db ) : Plugin( "test_plugin", db )
	{
	}

	virtual ~TestPlugin()
	{
	}

    public:
	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    REQUIRE( graph_.public_transports.size() >= 1 );
	    REQUIRE( request.check_consistency() );
	    REQUIRE( request.steps.size() == 1 );

	    if ( (request.optimizing_criteria[0] != CostDuration) && (request.optimizing_criteria[0] != CostDistance) )
	    {
		throw std::invalid_argument( "Unsupported optimizing criterion" );
	    }
	    
	    request_ = request;
	    result_.clear();
 	}
    };

    DECLARE_TEMPUS_PLUGIN( PtPlugin );
};



