#include <iostream>

#include "application.hh"
#include "common.hh"
#include "pgsql_importer.hh"

namespace Tempus
{
    Application* Application::instance()
    {
	static Application app_;
	return &app_;
    }

    void Application::connect( const std::string& db_options )
    {
	db_options_ = db_options;
	db_.connect( db_options_ );
    }

    void Application::pre_build_graph()
    {
    }

    void Application::build_graph()
    {
	// request the database
	PQImporter importer( db_options_ );
	TextProgression progression(50);
	std::cout << "Loading graph from database: " << std::endl;
	std::cout << "Importing constants ... " << std::endl;
	importer.import_constants();
	std::cout << "Importing graph ... " << std::endl;
	importer.import_graph( graph_, progression );

	// call post_build() and validate() on each registered plugin
	Plugin::PluginList::iterator it;
	for ( it = Plugin::plugin_list().begin(); it != Plugin::plugin_list().end(); it++ )
	{
	    it->second->post_build();
	    it->second->validate();
	}
    }


    Plugin* Application::load_plugin( const std::string& dll_name )
    {
	return Plugin::load( dll_name );
    }

    void Application::unload_plugin( Plugin* handle )
    {
	Plugin::unload( handle );
    }
};
