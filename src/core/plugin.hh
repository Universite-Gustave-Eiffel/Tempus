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
#include <stdexcept>

#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"
#include "db.hh"
#include "application.hh"

#ifdef _WIN32
  #define NOMINMAX
  #include <windows.h>
  #define EXPORT __declspec(dllexport)
  #define DLL_SUFFIX ".dll"
  #define DLL_PREFIX ""
#else
  #include <dlfcn.h>
  #define EXPORT
  #define DLL_SUFFIX ".so"
  #define DLL_PREFIX "./lib"
#endif

namespace Tempus
{
    ///
    /// Base class that has to be derived in plugins
    ///
    class Plugin
    {
    public:
	static Plugin* load( const std::string& dll_name );
	static void unload( Plugin* plugin );
	std::string get_name() const { return name_; }
	
    public:
	///
	/// Called when the plugin is loaded into memory (install)
	Plugin( const std::string& name, Db::Connection& db );
	
	///
	/// Called when the plugin is unloaded from memory (uninstall)
	virtual ~Plugin()
	{
	}
	
	///
	/// Called after graphs have been built in memory.
	/// A Db::Connection is passed to the plugin
	virtual void post_build();
	
	///
	/// Called in order to validate the in-memory structure.
	virtual void validate();
	
	///
	/// TODO: find a way to use a visitor
	virtual void accessor();

	///
	/// Cycle
	virtual void cycle();

	///
	/// Pre-process the user request.
	/// \param[in] request The request to preprocess.
	/// \throw std::invalid_argument Throws an instance of std::invalid_argument if the request cannot be processed by the current plugin.
	virtual void pre_process( Request& request ) throw (std::invalid_argument);

	///
	/// Process the last preprocessed user request.
	/// Must populates the 'result_' object.
	virtual void process();

	///
	/// Post-process the user request.
	virtual void post_process();

	///
	/// Result formatting
	virtual Result& result();

	///
	/// Cleanup method.
	virtual void cleanup();

    protected:
	///
	/// Graph extracted from the database
	MultimodalGraph& graph_;
	///
	/// User request
	Request request_;
	///
	/// Result
	Result result_;

	/// Name of this plugin
	std::string name_;

	/// Db connection
	Db::Connection& db_;
	
	void* module_;
    };
}; // Tempus namespace




///
/// Macro used inside plugins.
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( type ) \
    extern "C" EXPORT Tempus::Plugin* createPlugin( Db::Connection& db ) { return new type( db ); } \
    extern "C" EXPORT void deletePlugin(Tempus::Plugin* p_) { delete p_; }

#endif
