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
	
	static Plugin* load( const char* dll_name );
	static void unload( Plugin* handle );
	
	std::string get_name() const { return name_; }
	
    public:
	///
	/// Called when the plugin is loaded into memory (install)
	Plugin(std::string name) : name_(name)
	{
	}
	
	///
	/// Called when the plugin is unloaded from memory (uninstall)
	virtual ~Plugin()
	{
	}
	
	///
	/// Pre-build graphs in memory
	/// \param[in] options Options used to connect to the database.
	virtual void pre_build( const std::string& options = "" );
	///
	/// Build graphs in memory
	virtual void build();
	///
	/// Called after graphs have been built in memory.
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
	/// Process the user request.
	/// \param[in] request The request to process.
	/// Must populates the 'result_' object.
	virtual void process( Request& request );

	///
	/// Post-process the user request.
	virtual void post_process();

	///
	/// Result formatting
	virtual void result();

	///
	/// Cleanup method.
	virtual void cleanup();

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
	
	void* module_;
    };
}; // Tempus namespace




///
/// Macro used inside plugins.
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( type ) \
    extern "C" EXPORT Tempus::Plugin* createPlugin() { return new type(); } \
    extern "C" EXPORT void deletePlugin(Tempus::Plugin* p_) { delete p_; }

#endif
