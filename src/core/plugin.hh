// Tempus plugin architecture
// (c) 2012 Oslandia
// MIT License

#ifndef TEMPUS_PLUGIN_CORE_HH
#define TEMPUS_PLUGIN_CORE_HH

#include <string>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

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

// this is the type of pointer to dll
#ifndef _WIN32
#   define HMODULE void*
#endif

namespace Tempus
{

    /**
       Base class that has to be derived in plugins

       A Tempus plugin is made of :
       - some user-defined options
       - some callback functions called when user requests are processed
       - some performance metrics
    */
    class Plugin
    {
    public:

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
	/// Conversion from a C++ type to an OptionType.
	/// (Uses template specialization)
	template <typename T>
	struct OptionTypeFrom
	{
	    static const OptionType type;
	};

        typedef boost::any OptionValue;
        /*
        class OptionValue : public boost::any
        {
        public:
            OptionValue() : boost::any() {}
            template <typename T> OptionValue( T v ) : boost::any(v) { }
            OptionValue( const OptionValue & v ): boost::any(v), counter(v.counter+1)  { assert(counter < 10);}
     
            std::string to_string( ) const;
        };
        */
        static std::string to_string( const OptionValue & );


	typedef std::map<std::string, OptionValue> OptionValueList;

	///
	/// Plugin option description
	struct OptionDescription
	{
	    OptionType type;
	    std::string description;
	    OptionValue default_value;
	};

    struct OptionDescriptionList: std::map<std::string, OptionDescription>
    {
        ///
        /// Method used by a plugin to declare an option
        template <class T>
        void declare_option( const std::string& nname, const std::string& description, T default_value )
        {
            (*this)[nname].type = OptionTypeFrom<T>::type;
            (*this)[nname].description = description;
            (*this)[nname].default_value = OptionValue(default_value);
        }
        void set_options_default_value( Plugin * plugin )
        {
            for (iterator i=begin(); i!=end(); i++) plugin->set_option( i->first, i->second.default_value );
        }
    };

	OptionValueList& options() { return options_; }
	
	///
	/// Method used to set an option value
	template <class T>
	void set_option( const std::string& nname, const T& value )
	{
	    options_[nname] = value;
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
	void get_option( const std::string& name, T& value);

	///
	/// Method used to get an option value, alternative signature.
	template <class T>
	T get_option( const std::string& nname )
	{
	    T v;
	    get_option( nname, v );
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

	///
	/// Ctor
	Plugin( const std::string& name, const std::string & db_options );
	
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
	
	enum AccessType
	{
	    InitAccess,
	    DiscoverAccess,
	    ExamineAccess,
	    EdgeRelaxedAccess,
	    EdgeNotRelaxedAccess,
	    EdgeMinimizedAccess,
	    EdgeNotMinimizedAccess,
	    TreeEdgeAccess,
	    NonTreeEdgeAccess,
	    BackEdgeAccess,
	    ForwardOrCrossEdgeAccess,
	    StartAccess,
	    FinishAccess,
	    GrayTargetAccess,
	    BlackTargetAccess
	};
	///
	/// Acessors methods. They can be called on graph traversals.
	/// A Plugin is made compatible with a boost::visitor by means of a PluginGraphVisitor
	virtual void road_vertex_accessor( Road::Vertex, int /*access_type*/ ) {}
	virtual void road_edge_accessor( Road::Edge, int /*access_type*/ ) {}
	virtual void pt_vertex_accessor( PublicTransport::Vertex, int /*access_type*/ ) {}
	virtual void pt_edge_accessor( PublicTransport::Edge, int /*access_type*/ ) {}
	virtual void vertex_accessor( Multimodal::Vertex, int /*access_type*/ ) {}
	virtual void edge_accessor( Multimodal::Edge, int /*access_type*/ ) {}

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
	Multimodal::Graph & graph_;
	///
	/// User request
	Request request_;
	///
	/// Result
	Result result_;

	/// Name of the dll this plugin comes from
	std::string name_;

	/// Db connection
	Db::Connection db_; // each plugin has it's own connection
	
	/// Plugin option management
	OptionValueList options_;

	MetricValueList metrics_;
    };

    template <>
    struct Plugin::OptionTypeFrom<bool> { static const Plugin::OptionType type = Plugin::BoolOption; };
    template <>
    struct Plugin::OptionTypeFrom<int> { static const Plugin::OptionType type = Plugin::IntOption; };
    template <>
    struct Plugin::OptionTypeFrom<float> { static const Plugin::OptionType type = Plugin::FloatOption; };
    template <>
    struct Plugin::OptionTypeFrom<std::string> { static const Plugin::OptionType type = Plugin::StringOption; };

    ///
    /// Class used as a boost::visitor.
    /// This is a proxy to Plugin::xxx_accessor methods. It may be used as implementation of any kind of boost::graph visitors
    /// (BFS, DFS, Dijkstra, A*, Bellman-Ford)
    template <class Graph,
	      void (Plugin::*VertexAccessorFunction)(typename boost::graph_traits<Graph>::vertex_descriptor, int),
	      void (Plugin::*EdgeAccessorFunction)(typename boost::graph_traits<Graph>::edge_descriptor, int)
	      >	      
    class PluginGraphVisitorHelper
    {
    public:
	PluginGraphVisitorHelper( Plugin* plugin ) : plugin_(plugin) {}
	typedef typename boost::graph_traits<Graph>::vertex_descriptor VDescriptor;
	typedef typename boost::graph_traits<Graph>::edge_descriptor EDescriptor;
	
	void initialize_vertex( VDescriptor v, const Graph& )
	{
	    (plugin_->*VertexAccessorFunction)( v, Plugin::InitAccess );
	}
	void examine_vertex( VDescriptor v, const Graph& )
	{
	    (plugin_->*VertexAccessorFunction)( v, Plugin::ExamineAccess );
	}
	void discover_vertex( VDescriptor v, const Graph& )
	{
	    (plugin_->*VertexAccessorFunction)( v, Plugin::DiscoverAccess );
	}
	void start_vertex( VDescriptor v, const Graph& )
	{
	    (plugin_->*VertexAccessorFunction)( v, Plugin::StartAccess );
	}
	void finish_vertex( VDescriptor v, const Graph& )
	{
	    (plugin_->*VertexAccessorFunction)( v, Plugin::FinishAccess );
	}

	void examine_edge( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::ExamineAccess );
	}
	void tree_edge( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::TreeEdgeAccess );
	}
	void non_tree_edge( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::NonTreeEdgeAccess );
	}
	void back_edge( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::BackEdgeAccess );
	}
	void gray_target( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::GrayTargetAccess );
	}
	void black_target( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::BlackTargetAccess );
	}
	void forward_or_cross_edge( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::ForwardOrCrossEdgeAccess );
	}
	void edge_relaxed( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::EdgeRelaxedAccess );
	}
	void edge_not_relaxed( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::EdgeNotRelaxedAccess );
	}
	void edge_minimized( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::EdgeMinimizedAccess );
	}
	void edge_not_minimized( EDescriptor e, const Graph& )
	{
	    (plugin_->*EdgeAccessorFunction)( e, Plugin::EdgeNotMinimizedAccess );
	}

    protected:
	Plugin *plugin_;
    };

    typedef PluginGraphVisitorHelper< Road::Graph, &Plugin::road_vertex_accessor, &Plugin::road_edge_accessor > PluginRoadGraphVisitor;
    typedef PluginGraphVisitorHelper< PublicTransport::Graph, &Plugin::pt_vertex_accessor, &Plugin::pt_edge_accessor > PluginPtGraphVisitor;
    typedef PluginGraphVisitorHelper< Multimodal::Graph, &Plugin::vertex_accessor, &Plugin::edge_accessor > PluginGraphVisitor;

    /**
     * Handles dll so that they are only loaded once and unloaded
     * at the program termination (static plugin_factory)
     */
    struct PluginFactory: boost::noncopyable
    {
        ~PluginFactory();

        /** 
         * list loaded plugins
         */
        std::vector<std::string> plugin_list() const;

        
        const Plugin::OptionDescriptionList option_descriptions( const std::string & dll_name ) const;

        /**
         * create a Plugin from a given dll
         * the caller takes ownership
         * throws std::runtime_error if dll is not loaded
         */
        Plugin * createPlugin( const std::string & dll_name ) const;

        /**
         * throws std::runtime_error if dll cannot be found
         * if dll is already loaded, well do nothing
         */
        void load( const std::string& dll_name );

        static PluginFactory instance;
    private:

        struct Dll 
        {
            typedef Plugin*                              (*PluginCreationFct)( const std::string & );
            typedef const Plugin::OptionDescriptionList* (*PluginOptionDescriptionFct)();
            typedef const char*                          (*PluginNameFct)();
            HMODULE           handle_;
            PluginCreationFct create;
            PluginOptionDescriptionFct options_description;
        };

        typedef std::map<std::string, Dll > DllMap;
        DllMap dll_;
    };
    // use of volatile for shared resources is explaned here http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766


} // Tempus namespace




///
/// Macro used inside plugins.
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( name, type )                              \
    extern "C" EXPORT Tempus::Plugin* createPlugin( const std::string & db_options ) { return new type( name, db_options ); } \
    extern "C" EXPORT const char *    pluginName() { return name; } \
    extern "C" EXPORT const Tempus::Plugin::OptionDescriptionList* optionDescriptions( ) { return new Tempus::Plugin::OptionDescriptionList(type::option_descriptions()); } \
    extern "C" EXPORT void                                        deletePlugin(Tempus::Plugin* p_) { delete p_; }

#endif
