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
	// default metrics
	metrics_[ "time_s" ] = 0.0;
	metrics_[ "iterations" ] = 0;
    }

    void Plugin::declare_option( const std::string& name, OptionType type, const std::string& description )
    {
	options_descriptions_[name].type = type;
	options_descriptions_[name].description = description;
	options_[name] = boost::any();
    }
    
    void Plugin::set_option_from_string( const std::string& name, const std::string& value)
    {
	if ( options_descriptions_.find( name ) == options_descriptions_.end() )
	    return;
	OptionType t = options_descriptions_[name].type;
	switch (t)
	{
	case BoolOption:
	    options_[name] = boost::lexical_cast<int>( value ) == 0 ? false : true;
	    break;
	case IntOption:
	    options_[name] = boost::lexical_cast<int>( value );
	    break;
	case FloatOption:
	    options_[name] = boost::lexical_cast<float>( value );
	    break;
	case StringOption:
	    options_[name] = value;
	    break;
	default:
	    throw std::runtime_error( "Unknown type" );
	}
    }

    std::string Plugin::option_to_string( const std::string& name )
    {
	if ( options_descriptions_.find( name ) == options_descriptions_.end() )
	{
	    throw std::invalid_argument( "Cannot find option " + name );
	}
	OptionType t = options_descriptions_[name].type;
	boost::any value = options_[name];
	if ( value.empty() )
	    return "";

	switch (t)
	{
	case BoolOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<bool>(value) ? 1 : 0 );
	case IntOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<int>(value) );
	case FloatOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<float>(value) );
	case StringOption:
	    return boost::any_cast<std::string>(value);
	default:
	    throw std::runtime_error( "Unknown type" );
	}
	// never happens
	return "";
    }

    std::string Plugin::metric_to_string( const std::string& name )
    {
	if ( metrics_.find( name ) == metrics_.end() )
	{
	    throw std::invalid_argument( "Cannot find metric " + name );
	}
	boost::any v = metrics_[name];
	if ( v.empty() )
	    return "";

	if ( v.type() == typeid( bool ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<bool>( v ) ? 1 : 0 );
	}
	else if ( v.type() == typeid( int ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<int>( v ) );
	}
	else if ( v.type() == typeid( float ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<float>( v ) );
	}
	else if ( v.type() == typeid( std::string ) )
	{
	    return boost::any_cast<std::string>( v );
	}
	throw std::invalid_argument( "No known conversion for metric " + name );
	// never happens
	return "";
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

