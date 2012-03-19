#include <string>
#include <iostream>

#include "plugin.hh"

typedef Tempus::Plugin* (*PluginCreationFct)();
typedef void (*PluginDeletionFct)(Tempus::Plugin*);

namespace Tempus
{
    Plugin* Plugin::load( const char* dll_name )
    {
	std::string complete_dll_name = DLL_PREFIX + std::string( dll_name ) + DLL_SUFFIX;
	std::cout << "Loading " << complete_dll_name << std::endl;
#ifdef _WIN32
	HMODULE h = LoadLibrary( complete_dll_name.c_str() );
	if ( h == NULL )
	{
	    std::cerr << "DLL loading problem: " << GetLastError() << std::endl;
	    return 0;
	}
	PluginCreationFct createFct = (PluginCreationFct) GetProcAddress( h, "createPlugin" );
	if ( createFct == NULL )
	{
	    std::cerr << "DLL loading problem: " << GetLastError() << std::endl;
	    FreeLibrary( h );
	    return 0;
	}
#else
	void *h = dlopen( complete_dll_name.c_str(), RTLD_NOW );
	if ( !h )
	{
	    std::cerr << dlerror() << std::endl;
	    return 0;
	}
	PluginCreationFct createFct = (PluginCreationFct)dlsym( h, "createPlugin" );
	if ( createFct == 0 )
	{
	    std::cerr << dlerror() << std::endl;
	    dlclose( h );
	    return 0;
	}
#endif
	Tempus::Plugin* plugin = createFct();
	std::cout << "plugin = " << plugin << std::endl;
	plugin->module_ = h;
	return plugin;
    }

    void Plugin::unload( Plugin* handle )
    {
	if ( handle )
	{
	    ///
	    /// We cannot call delete directly on the plugin pointer, since it has been allocated from within another DLL.
#ifdef _WIN32
	    PluginDeletionFct deleteFct = (PluginDeletionFct) GetProcAddress( (HMODULE)handle->module_, "deletePlugin" );
	    FreeLibrary( (HMODULE)handle->module_ );
#else
	    PluginDeletionFct deleteFct = (PluginDeletionFct) dlsym( handle->module_, "deletePlugin" );
	    dlclose( handle->module_ );
#endif
	    deleteFct(handle);
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

