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
};

