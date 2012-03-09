// Tempus plugin architecture
// (c) 2012 Oslandia
// MIT License

/**
   Tempus plugin architecture
 */

#ifndef TEMPUS_PLUGIN_CORE_HH
#define TEMPUS_PLUGIN_CORE_HH

#include <string>
#include <iostream>

#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"

namespace Tempus
{
    ///
    /// Base class that has to be derived in plugins
    ///
    class Plugin
    {
    public:
	static std::list<Plugin*> plugins;

	static void* load( const char* dll_name );
	static void unload( void* handle );

	std::string get_name() const { return name_; }

    public:
	///
	/// Called when the plugin is loaded into memory (install)
	Plugin(std::string name) : name_(name)
	{
	    std::cout << name_ << " constructor" << std::endl;
	    // TODO : look for existing plugin of the same name
	    plugins.push_back( this );
	}

	///
	/// Called when the plugin is unloaded from memory (uninstall)
	virtual ~Plugin()
	{
	    std::list<Plugin*>::iterator it;
	    it = std::find( plugins.begin(), plugins.end(), this );
	    plugins.erase( it );
	    std::cout << name_ << " destructor" << std::endl;
	}

	virtual void pre_build()
	{
	    std::cout << "[" << name_ << "]: " << "pre_build" << std::endl;
	}
	virtual void build( /* databse request */)
	{
	    std::cout << "[" << name_ << "]: " << "build" << std::endl;
	}
	virtual void post_build()
	{
	    std::cout << "[" << name_ << "]: " << "post_build" << std::endl;
	}

	virtual void validate()
	{
	    std::cout << "[" << name_ << "]: " << "validate" << std::endl;
	}

	///
	/// ???
	virtual void accessor()
	{
	    std::cout << "[" << name_ << "]: " << "accessor" << std::endl;
	}

	virtual void pre_process()
	{
	    std::cout << "[" << name_ << "]: " << "pre_process" << std::endl;
	}

	///
	/// Process the user request.
	/// Must populates the 'result_' object.
	virtual void process(Request& request)
	{
	    request_ = request;
	    std::cout << "[" << name_ << "]: " << "process" << std::endl;
	}

	virtual void post_process()
	{
	    std::cout << "[" << name_ << "]: " << "post_process" << std::endl;
	}

	///
	/// ??? text formatting ?
	virtual void result()
	{
	    std::cout << "[" << name_ << "]: " << "result" << std::endl;
	}

    protected:
	///
	/// Graph extracted from the database
	MultimodalGraph graph_;
	///
	/// User request
	Request request_;
	///
	/// Result
	Result result_;

	/// Name of this plugin
	std::string name_;
    };
}; // Tempus namespace


///
/// Macro used inside plugins.
/// This way, constructor will be called on library loading and destructor on library unloadings
#define DECLARE_TEMPUS_PLUGIN( type ) static Tempus::type plugin_instance_

#endif
