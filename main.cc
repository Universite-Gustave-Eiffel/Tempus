#include <boost/graph/depth_first_search.hpp>
#include <iostream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;
using namespace Tempus;

int main()
{
    ///
    /// Plugins

    void* h = Tempus::Plugin::load( "dummy_plugin" );

    for (list<Tempus::Plugin*>::iterator it = Tempus::Plugin::plugins.begin(); it != Tempus::Plugin::plugins.end(); it++ )
    {
	cout << "[plugin " << (*it)->get_name() << "]" << endl;

	cout << endl << ">> pre_build" << endl;
	(*it)->pre_build();
	cout << endl << ">> build" << endl;
	(*it)->build();
	cout << endl << ">> post_build" << endl;
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

	cout << endl << ">> pre_process" << endl;
	(*it)->pre_process();
	cout << endl << ">> process" << endl;
	(*it)->process( req );
	cout << endl << ">> post_process" << endl;
	(*it)->post_process();

	cout << endl << ">> result" << endl;
	(*it)->result();
    }

    if ( h )
    {
	Tempus::Plugin::unload( h );
    }

    return 0;
}
