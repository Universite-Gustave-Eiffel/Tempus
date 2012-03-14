#include <boost/graph/depth_first_search.hpp>
#include <iostream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;
using namespace Tempus;

namespace PT = Tempus::PublicTransport;

struct MyVisitor
{
    void initialize_vertex( PT::Vertex s, PT::Graph g )
    {
    }
    void start_vertex( PT::Vertex s, PT::Graph g )
    {
    }
    void discover_vertex( PT::Vertex s, PT::Graph g )
    {
	cout << "Discovering " << g[s].name << endl;
    };
    void finish_vertex( PT::Vertex s, PT::Graph g )
    {
    };
    
    void examine_edge( PT::Edge e, PT::Graph )
    {
    }
    void tree_edge( PT::Edge e, PT::Graph )
    {
    }
    void back_edge( PT::Edge e, PT::Graph )
    {
    }
    void forward_or_cross_edge( PT::Edge e, PT::Graph )
    {
    }
};

int main()
{
    ///
    /// Plugins

    void* h = Tempus::Plugin::load( "dummy_plugin" );

    for (std::list<Tempus::Plugin*>::iterator it = Tempus::Plugin::plugins.begin(); it != Tempus::Plugin::plugins.end(); it++ )
    {
	std::cout << "plugin " << (*it)->get_name() << std::endl;

	(*it)->pre_build();
	(*it)->build();
	(*it)->post_build();

	//
	// Build the user request
	MultimodalGraph* graph = (*it)->get_graph();
	Road::Graph& road_graph = graph->road;

	Road::VertexIterator vb, ve;
	boost::tie( vb, ve) = boost::vertices( road_graph );
	ve--;

	// go from the first road node, to the last one
	Request req;
	req.origin = *vb;
	Request::Step step;
	step.destination = *ve;
	req.steps.push_back( step );

	// the only optimizing criterion
	req.optimizing_criteria.push_back( CostDuration );

	(*it)->pre_process();
	(*it)->process( req );
	(*it)->post_process();

	// TODO: result processing
    }

    if ( h )
    {
	Tempus::Plugin::unload( h );
    }

    return 0;
}
