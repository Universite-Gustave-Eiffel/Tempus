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
	}

	virtual void pre_build();
	virtual void build();
	virtual void post_build();

	virtual void validate();

	///
	/// TODO: find a way to use a visitor
	virtual void accessor();

	virtual void pre_process();

	///
	/// Process the user request.
	/// Must populates the 'result_' object.
	virtual void process( /* IN */ Request& request);

	virtual void post_process();

	///
	/// Result formatting
	virtual void result();

	MultimodalGraph* get_graph() { return &graph_; }
	Result* get_result() { return &result_; }

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
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( type ) static Tempus::type plugin_instance_

#endif
