#include <iostream>

#include "application.hh"
#include "common.hh"
#include "pgsql_importer.hh"

Tempus::Application* get_application_instance_()
{
	return Tempus::Application::instance();
}

namespace Tempus
{
    Application* Application::instance()
    {
	// On Windows, static and global variables are COPIED from the main module (EXE) to the other (DLL).
	// DLL have still access to the main EXE memory ...
	static Application* instance_ = 0;
	if ( 0 == instance_ )
	{
#ifdef _WIN32
	    // We test if we are in the main module (EXE) or not. If it is the case, a new Application is allocated.
	    // It will also be returned by modules.
	    Application* (*main_get_instance)() = (Application* (*)())GetProcAddress(GetModuleHandle(NULL), "get_application_instance_");
	    if ( main_get_instance == &get_application_instance_ )
	    {
		instance_ = new Application();
	    }
	    else
	    {
		instance_ = main_get_instance();
	    }
#else
	    instance_ = new Application();
#endif
	}
	return instance_;
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
