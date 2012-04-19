// Tempus plugin architecture
// (c) 2012 Oslandia
// MIT License

/**
   A Tempus plugin is made of :
   - some user-defined options
   - some callback functions called when user requests are processed
   - some performance metrics
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
	///
	/// Static function used to load a plugin from disk
	static Plugin* load( const std::string& dll_name );
	///
	/// Static funtion used to unload a plugin
	static void unload( Plugin* plugin );			     

	///
	/// Access to global plugin list
	typedef std::map<std::string, Plugin*> PluginList;
	static PluginList& plugin_list() { return plugin_list_; }

	///
	/// Plugin option type
	enum OptionType
	{
	    BoolOption,
	    IntOption,
	    FloatOption,
	    StringOption
	};
	///
	/// Plugin option description
	struct OptionDescription
	{
	    OptionType type;
	    std::string description;
	    bool mandatory;
	};
	typedef std::map<std::string, OptionDescription> OptionDescriptionList;
	typedef boost::any OptionValue;
	typedef std::map<std::string, OptionValue> OptionValueList;

	///
	/// Method used by a plugin to declare an option
	void declare_option( const std::string& name, OptionType type, const std::string& description );

	///
	/// Option descriptions accessor
	OptionDescriptionList& option_descriptions()
	{
	    return options_descriptions_;
	}
	OptionValueList& options() { return options_; }
	
	///
	/// Method used to set an option value
	template <class T>
	void set_option( const std::string& name, const T& value )
	{
	    options_[name] = value;
	}
	///
	/// Method used to set an option value from a string. Conversions are made, based on the option description
	void set_option_from_string( const std::string& name, const std::string& value);
	///
	/// Method used to get a string from an option value
	std::string option_to_string( const std::string& name );

	///
	/// Method used to get an option value
	template <class T>
	void get_option( const std::string& name, T& value)
	{
	    if ( options_.find( name ) == options_.end() )
	    {
		throw std::runtime_error( "get_option(): cannot find option " + name );
	    }
	    value = boost::any_cast<T>(options_[name]);
	}
	///
	/// Method used to get an option value, alternative signature.
	template <class T>
	T get_option( const std::string& name )
	{
	    T v;
	    get_option( name, v );
	    return v;
	}

	///
	/// A metric is also a boost::any
	typedef boost::any MetricValue;
	///
	/// Metric name -> value
	typedef std::map<std::string, MetricValue> MetricValueList;
	
	///
	/// Access to metric list
	MetricValueList& metrics() { return metrics_; }
	///
	/// Converts a metric value to a string
	std::string metric_to_string( const std::string& name );
	
	///
	/// Name accessor
	std::string name() const { return name_; }
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
	virtual void post_build();
	
	///
	/// Called in order to validate the in-memory structure.
	virtual void validate();
	
	///
	/// TODO: find a way to use a visitor, use overriding with visitor filter event tag
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
	
	static PluginList plugin_list_;
	/// The concrete plugin handler (HMODULE or void*)
	void* module_;

	/// Plugin option management
	OptionDescriptionList options_descriptions_;
	OptionValueList options_;

	MetricValueList metrics_;
    };
}; // Tempus namespace




///
/// Macro used inside plugins.
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( type ) \
    extern "C" EXPORT Tempus::Plugin* createPlugin( Db::Connection& db ) { return new type( db ); } \
    extern "C" EXPORT void deletePlugin(Tempus::Plugin* p_) { delete p_; }

#endif
