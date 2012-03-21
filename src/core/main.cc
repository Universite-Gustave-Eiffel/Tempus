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
    try
    {
	Plugin* plugin = Tempus::Plugin::load( "dummy_plugin" );
	
	cout << "[plugin " << plugin->get_name() << "]" << endl;
	
	cout << endl << ">> pre_build" << endl;
	plugin->pre_build();
	cout << endl << ">> build" << endl;
	plugin->build();
	cout << endl << ">> post_build" << endl;
	plugin->post_build();

	//
	// Build the user request
	MultimodalGraph* graph = plugin->get_graph();
	Road::Graph& road_graph = graph->road;

	Road::VertexIterator vb, ve;
	boost::tie( vb, ve) = boost::vertices( road_graph );
	ve--;

	// go from the first road node, to the last one
	Request req;
	Request::TransportSelection tsel;
	tsel.type = Tempus::transport_type_from_name[ "Tramway" ];
	// allow only one transport network
	tsel.allowed_networks.push_back( 1 );
	req.origin = *vb;
	Request::Step step;
	step.destination = *ve;
	step.allowed_transport_types.push_back( tsel );
	req.steps.push_back( step );

	// the only optimizing criterion
	req.optimizing_criteria.push_back( CostDuration );

	cout << endl << ">> pre_process" << endl;
	plugin->pre_process();
	cout << endl << ">> process" << endl;
	plugin->process( req );
	cout << endl << ">> post_process" << endl;
	plugin->post_process();

	cout << endl << ">> result" << endl;
	plugin->result();
	
	Tempus::Plugin::unload( plugin );
    }
    catch (std::exception& e)
    {
	cerr << "Exception: " << e.what() << endl;
    }
    
    return 0;
}
