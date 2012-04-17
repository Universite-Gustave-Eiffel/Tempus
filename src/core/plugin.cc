#include <string>
#include <iostream>

#include "plugin.hh"

typedef Tempus::Plugin* (*PluginCreationFct)( Db::Connection& );
typedef void (*PluginDeletionFct)(Tempus::Plugin*);

namespace Tempus
{
    Plugin::PluginList Plugin::plugin_list_;

    Plugin::Plugin( const std::string& name, Db::Connection& db ) :
	name_(name),
	db_(db),
	graph_(Application::instance()->graph())
    {
    }

    void Plugin::declare_option( const std::string& name, OptionType type, const std::string& description )
    {
	options_descriptions_[name].type = type;
	options_descriptions_[name].description = description;
    }
    
    Plugin* Plugin::load( const std::string& dll_name )
    {
	std::string complete_dll_name = DLL_PREFIX + dll_name + DLL_SUFFIX;
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
	    std::cerr << "GetProcAddress problem: " << GetLastError() << std::endl;
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
	Tempus::Plugin* plugin = createFct( Application::instance()->db_connection() );
	plugin->module_ = h;
	plugin_list_[dll_name] = plugin;
	return plugin;
    }

    void Plugin::unload( Plugin* handle )
    {
	if ( handle )
	{
	    PluginList::iterator it;
	    for ( it = plugin_list_.begin(); it != plugin_list_.end(); it++ )
	    {
		if ( it->second == handle )
		{
		    plugin_list_.erase( it );
		    break;
		}
	    }

	    ///
	    /// We cannot call delete directly on the plugin pointer, since it has been allocated from within another DLL.
#ifdef _WIN32
	    PluginDeletionFct deleteFct = (PluginDeletionFct) GetProcAddress( (HMODULE)handle->module_, "deletePlugin" );
	    HMODULE module = (HMODULE)handle->module_;
	    deleteFct(handle);
	    FreeLibrary( module );
#else
	    PluginDeletionFct deleteFct = (PluginDeletionFct) dlsym( handle->module_, "deletePlugin" );
	    void* module = handle->module_;
	    deleteFct(handle);
	    if ( dlclose( module ) )
	    {
		std::cerr << "Error on dlclose " << dlerror() << std::endl;
	    }
#endif
	}
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

    void Plugin::cycle()
    {
	std::cout << "[plugin_base]: cycle" << std::endl;
    }

    void Plugin::pre_process( Request& request ) throw (std::invalid_argument)
    {
	std::cout << "[plugin_base]: pre_process" << std::endl;
	request_ = request;
    }

    ///
    /// Process the user request.
    /// Must populates the 'result_' object.
    void Plugin::process()
    {
	std::cout << "[plugin_base]: process" << std::endl;
    }

    void Plugin::post_process()
    {
	std::cout << "[plugin_base]: post_process" << std::endl;
    }

    ///
    /// ??? text formatting ?
    Result& Plugin::result()
    {
	std::cout << "[plugin_base]: result" << std::endl;
	return result_;
    }

    void Plugin::cleanup()
    {
	std::cout << "[plugin_base]: cleanup" << std::endl;
    }
};

