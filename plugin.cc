#include <dlfcn.h>
#include <string>
#include <iostream>

#include "plugin.hh"

namespace Tempus
{
    std::list<Plugin*> Plugin::plugins;

    void* Plugin::load( const char* dll_name )
    {
	std::string complete_dll_name = "./lib" + std::string( dll_name ) + ".so";
	std::cout << "Loading " << complete_dll_name << std::endl;
	void *h = dlopen( complete_dll_name.c_str(), RTLD_NOW );
	if ( !h )
	{
	    std::cerr << dlerror() << std::endl;
	}
	return h;
    }

    void Plugin::unload( void* handle )
    {
	if ( handle )
	{
	    dlclose( handle );
	}
    }

    void Plugin::pre_build()
    {
	std::cout << "[plugin_base]: pre_build" << std::endl;
    }

    void Plugin::build( /* database request */)
    {
	std::cout << "[plugin_base]: build" << std::endl;
    }

    void Plugin::post_build()
    {
	std::cout << "[plugin_base]: post_build" << std::endl;
    }

    void Plugin::validate()
    {
	std::cout << "[plugin_base]: validate" << std::endl;
    }

    ///
    /// ???
    void Plugin::accessor()
    {
	std::cout << "[plugin_base]: accessor" << std::endl;
    }

    void Plugin::pre_process()
    {
	std::cout << "[plugin_base]: pre_process" << std::endl;
    }

    ///
    /// Process the user request.
    /// Must populates the 'result_' object.
    void Plugin::process(Request& request)
    {
	request_ = request;
	std::cout << "[plugin_base]: process" << std::endl;
    }

    void Plugin::post_process()
    {
	std::cout << "[plugin_base]: post_process" << std::endl;
    }

    ///
    /// ??? text formatting ?
    void Plugin::result()
    {
	std::cout << "[plugin_base]: result" << std::endl;
    }

};

