#include "plugin.hh"

#include "pgsql_importer.hh"

namespace Tempus
{
    class MyPlugin : public Plugin
    {
    public:
	MyPlugin() : Plugin("myplugin")
	{
	}

	virtual ~MyPlugin()
	{
	}

	virtual void build()
	{
	    // request the database
	    PQImporter importer( "dbname = tempus" );
	    importer.import_graph( graph_ );
	}

	virtual void process( Request& request )
	{
	    request_ = request;
	    BOOST_ASSERT( request.check_consistency() );
	    if ( request.optimizing_criteria[0] != CostDuration )
	    {
		throw std::runtime_error( "Unsupported optimizing criterion" );
	    }
	    PublicTransport::Graph pt_graph = graph_.public_transports.front();
	    Road::Graph& road_graph = graph_.road;

	    // for each step in the request, find the corresponding public transport node
	    
	    for ( size_t i = 0; i < request.steps.size(); i++ )
	    {
		Road::Vertex node = request.steps[i];
		
		bool found = false;
		PublicTransport::Vertex found_vertex;

		PublicTransport::VertexIterator vb, ve;
		for ( boost::tie(vb, ve) = boost::vertices( pt_graph ); vb != ve; vb++ )
		{
		    PublicTransport::Stop& stop = pt_graph[*vb];
		    Road::Edge section = stop.road_section;
		    Road::Vertex s, t;
		    s = boost::source( section, road_graph );
		    t = boost::target( section, road_graph );
		    if ( (node == s) || (node == t) )
		    {
			found_vertex = *vb;
			found = true;
			break;
		    }
		}
		
		if (found)
		{
		    std::cout << "Road node #" << node << " <-> Public transport node '" << pt_graph[found_vertex].name << "'" << std::endl;
		}
	    }
	    // minimize duration
	}
    };
};

DECLARE_TEMPUS_PLUGIN( MyPlugin );

