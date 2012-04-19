// Tempus core main
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

#include <boost/program_options.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <iostream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;
using namespace Tempus;

namespace po = boost::program_options;

int main( int argc, char* argv[])
{
    // default db options
    string db_options = "dbname=tempus";
    string plugin_name = "sample_road_plugin";
    Tempus::db_id_t origin_id = 152500201343750;
    Tempus::db_id_t destination_id = 152500201357468;

    // parse command line arguments
    po::options_description desc("Allowed options");
    desc.add_options()
	("help", "produce help message")
	("db", po::value<string>(), "set database connection options")
	("plugin", po::value<string>(), "set the plugin name to launch")
	("origin", po::value<Tempus::db_id_t>(), "set the origin vertex id")
	("destination", po::value<Tempus::db_id_t>(), "set the destination vertex id")
	;
    
    po::variables_map vm;
    po::store(po::parse_command_line( argc, argv, desc ), vm);
    po::notify(vm);
    
    if ( vm.count("help") )
    {
	cout << desc << "\n";
	return 1;
    }

    if ( vm.count("db") )
    {
	db_options = vm["db"].as<string>();
    }
    if ( vm.count("plugin") )
    {
	plugin_name = vm["plugin"].as<string>();
    }
    if ( vm.count("origin") )
    {
	origin_id = vm["origin"].as<Tempus::db_id_t>();
    }
    if ( vm.count("destination") )
    {
	destination_id = vm["destination"].as<Tempus::db_id_t>();
    }

    Tempus::Application* app = Tempus::Application::instance();
    app->connect( db_options );
    ///
    /// Plugins
    try
    {
	Plugin* plugin = app->load_plugin( plugin_name );
	
	cout << "[plugin " << plugin->name() << "]" << endl;
	
	cout << endl << ">> pre_build" << endl;
	app->pre_build_graph();
	cout << endl << ">> build" << endl;
	app->build_graph();

	//
	// Build the user request
	MultimodalGraph& graph = app->graph();
	Road::Graph& road_graph = graph.road;

	Road::VertexIterator vb, ve;
	boost::tie( vb, ve) = boost::vertices( road_graph );
	ve--;

	Request req;
	Request::Step step;
	Road::VertexIterator vi, vi_end;
	bool found_origin = false;
	bool found_destination = false;
	for ( boost::tie(vi, vi_end) = boost::vertices(road_graph); vi != vi_end; vi++ )
	{
	    if (road_graph[*vi].db_id == origin_id )
	    {
		req.origin = *vi;
		found_origin = true;
		break;
	    }
	}
	if ( !found_origin )
	{
	    cerr << "Cannot find origin vertex ID " << origin_id << endl;
	    return 1;
	}
	for ( boost::tie(vi, vi_end) = boost::vertices(road_graph); vi != vi_end; vi++ )
	{
	    if (road_graph[*vi].db_id == destination_id )
	    {
		step.destination = *vi;
		found_destination = true;
		break;
	    }
	}
	if ( !found_destination )
	{
	    cerr << "Cannot find destination vertex ID " << destination_id << endl;
	    return 1;
	}

	req.steps.push_back( step );

	// the only optimizing criterion
	req.optimizing_criteria.push_back( CostDistance );

	cout << endl << ">> pre_process" << endl;
	try
	{
	    plugin->pre_process( req );
	}
	catch ( std::invalid_argument& e )
	{
	    std::cerr << "Can't process request : " << e.what() << std::endl;
	    return 1;
	}
	cout << endl << ">> process" << endl;
	plugin->process();
	cout << endl << ">> post_process" << endl;
	plugin->post_process();

	cout << endl << ">> result" << endl;
	plugin->result();

	cout << endl << ">> cleanup" << endl;
	plugin->cleanup();
	
	Tempus::Plugin::unload( plugin );
    }
    catch (std::exception& e)
    {
	cerr << "Exception: " << e.what() << endl;
    }
    
    return 0;
}
