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
	std::cout << "[" << name_ << "]: " << "pre_build" << std::endl;
    }

    void Plugin::build( /* database request */)
    {
	std::cout << "[" << name_ << "]: " << "build" << std::endl;
    }

    void Plugin::post_build()
    {
	std::cout << "[" << name_ << "]: " << "post_build" << std::endl;
    }

    void Plugin::validate()
    {
	std::cout << "[" << name_ << "]: " << "validate" << std::endl;
    }

    ///
    /// ???
    void Plugin::accessor()
    {
	std::cout << "[" << name_ << "]: " << "accessor" << std::endl;
    }

    void Plugin::pre_process()
    {
	std::cout << "[" << name_ << "]: " << "pre_process" << std::endl;
    }

    ///
    /// Process the user request.
    /// Must populates the 'result_' object.
    void Plugin::process(Request& request)
    {
	request_ = request;
	std::cout << "[" << name_ << "]: " << "process" << std::endl;
    }

    void Plugin::post_process()
    {
	std::cout << "[" << name_ << "]: " << "post_process" << std::endl;
    }

    ///
    /// ??? text formatting ?
    void Plugin::result()
    {
	std::cout << "[" << name_ << "]: " << "result" << std::endl;
    }

};

