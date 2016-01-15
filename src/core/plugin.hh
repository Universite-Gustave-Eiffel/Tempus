/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_PLUGIN_CORE_HH
#define TEMPUS_PLUGIN_CORE_HH

#include <string>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/thread.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"
#include "db.hh"
#include "application.hh"
#include "variant.hh"

#ifdef _WIN32
#   define NOMINMAX
#   include <windows.h>
#   define EXPORT __declspec(dllexport)
#   define DLL_SUFFIX ".dll"
#   define DLL_PREFIX ""
#else
#   include <dlfcn.h>
#   define EXPORT
#   define DLL_SUFFIX ".so"
#   define DLL_PREFIX "./lib"
#endif

// this is the type of pointer to dll
#ifndef _WIN32
#   define HMODULE void*
#endif

namespace Tempus {

class Plugin;

class PluginRequest
{
public:
    ///
    /// Constructor of the request
    /// \param[in] plugin The plugin that has created this request
    /// \param[in] options Option values for this request
    PluginRequest( const Plugin* plugin, const VariantMap& options );

    virtual ~PluginRequest() {}

    ///
    /// A metric is also an OptionValue
    typedef Variant MetricValue;
    ///
    /// Metric name -> value
    typedef std::map<std::string, MetricValue> MetricValueList;

    ///
    /// Access to metric list
    MetricValueList& metrics() {
        return metrics_;
    }
    ///
    /// Converts a metric value to a string
    std::string metric_to_string( const std::string& name ) const;

    enum AccessType {
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
    virtual void road_vertex_accessor( const Road::Vertex&, int /*access_type*/ ) {}
    virtual void road_edge_accessor( const Road::Edge&, int /*access_type*/ ) {}
    virtual void pt_vertex_accessor( const PublicTransport::Vertex&, int /*access_type*/ ) {}
    virtual void pt_edge_accessor( const PublicTransport::Edge&, int /*access_type*/ ) {}
    virtual void vertex_accessor( const Multimodal::Vertex&, int /*access_type*/ ) {}
    virtual void edge_accessor( const Multimodal::Edge&, int /*access_type*/ ) {}

    ///
    /// Pre-process the user request.
    /// \param[in] request The request to preprocess.
    /// \throw std::invalid_argument Throws an instance of std::invalid_argument if the request cannot be processed by the current plugin.
    /// \return a result, set of paths
    virtual std::unique_ptr<Result> process( const Request& request );

protected:
    /// The parent plugin
    const Plugin* plugin_;

    /// Plugin option management
    VariantMap options_;

    /// Plugin metrics
    MetricValueList metrics_;

private:
    ///
    /// Method used to get an option value
    template <class T>
    void get_option( const std::string& name, T& value ) const;

protected:
    ///
    /// Convenience method
    bool get_bool_option( const std::string& name ) const;

    ///
    /// Convenience method
    int64_t get_int_option( const std::string& name ) const;

    ///
    /// Convenience method
    double get_float_option( const std::string& name ) const;

    ///
    /// Convenience method
    std::string get_string_option( const std::string& name ) const;

    ///
    /// Method used to get an option value, alternative signature.
    template <class T>
    T get_option( const std::string& nname ) const {
        T v;
        get_option( nname, v );
        return v;
    }
};


/**
   Base class that has to be derived in plugins

   A Tempus plugin is made of :
   - some user-defined options
   - some callback functions called when user requests are processed
   - some performance metrics
*/
class Plugin {
public:

    ///
    /// Plugin option description
    struct OptionDescription {
        std::string description;
        Variant default_value;
        VariantType type() const {
            return default_value.type();
        }
        bool visible;
    };

    struct OptionDescriptionList : public std::map<std::string, OptionDescription>
    {
        ///
        /// Method used by a plugin to declare an option
        void declare_option( const std::string& nname, const std::string& description, const Variant& default_value, bool visible = true ) {
            ( *this )[nname].description = description;
            ( *this )[nname].default_value = default_value;
            ( *this )[nname].visible = visible;
        }
    };

    ///
    /// Get plugin option descriptions available for all plugins
    static OptionDescriptionList common_option_descriptions();

    ///
    /// Stores plugin parameters
    struct Capabilities {
        /// List of supported optimizing criteria
    private:
        std::vector<CostId> optimization_criteria_;
    public:
        std::vector<CostId>& optimization_criteria() { return optimization_criteria_; }
        const std::vector<CostId>& optimization_criteria() const { return optimization_criteria_; }

        /// Does the plugin support intermediate steps ?
        DECLARE_RW_PROPERTY( intermediate_steps, bool );

        /// Does the plugin support 'depart after' time constraints ?
        DECLARE_RW_PROPERTY( depart_after, bool );

        /// Does the plugin support 'arrive before' time constraints ?
        DECLARE_RW_PROPERTY( arrive_before, bool );

        Capabilities() :
            intermediate_steps_( false ),
            depart_after_( false ),
            arrive_before_( false ) {}
    };

    ///
    /// Name accessor
    std::string name() const {
        return name_;
    }

    ///
    /// Get value of the option, or the default value

    ///
    /// Ctor
    Plugin( const std::string& name, const VariantMap& = VariantMap() );

    ///
    /// Called when the plugin is unloaded from memory (uninstall)
    virtual ~Plugin() {}

    ///
    /// Create a PluginRequest object
    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const = 0;

    ///
    /// Gets access to the underlying routing data
    virtual const RoutingData* routing_data() const = 0;

    ///
    /// Gets an option value, or the default value if unavailable
    Variant get_option_or_default( const VariantMap& options, const std::string& key ) const;

    std::string db_options() const { return db_options_; }
    std::string schema_name() const { return schema_name_; }

private:
    /// Name of the dll this plugin comes from
    std::string name_;

    OptionDescriptionList option_descriptions_;
    Capabilities capabilities_;

    std::string db_options_;
    std::string schema_name_;
};


///
/// Convenience method to get plugin options
Plugin::OptionDescriptionList option_descriptions( const Plugin* plugin );

///
/// Helper function to fill the missing fields of a result (roadmaps) thanks to the database
/// \param result The result that is read and augmented (road names, geometries, etc.)
/// \param[in] connection The db connection to get additional information from
    void simple_multimodal_roadmap( Result& result, Db::Connection& connection, const Multimodal::Graph& graph );

///
/// Class used as a boost::visitor.
/// This is a proxy to Plugin::xxx_accessor methods. It may be used as implementation of any kind of boost::graph visitors
/// (BFS, DFS, Dijkstra, A*, Bellman-Ford)
template <class Graph,
         void ( PluginRequest::*VertexAccessorFunction )( const typename boost::graph_traits<Graph>::vertex_descriptor&, int ),
         void ( PluginRequest::*EdgeAccessorFunction )( const typename boost::graph_traits<Graph>::edge_descriptor&, int )
         >
class PluginGraphVisitorHelper {
public:
    PluginGraphVisitorHelper( PluginRequest* plugin ) : plugin_( plugin ) {}
    typedef typename boost::graph_traits<Graph>::vertex_descriptor VDescriptor;
    typedef typename boost::graph_traits<Graph>::edge_descriptor EDescriptor;

    void initialize_vertex( const VDescriptor& v, const Graph& ) {
        ( plugin_->*VertexAccessorFunction )( v, PluginRequest::InitAccess );
    }
    void examine_vertex( const VDescriptor& v, const Graph& ) {
        ( plugin_->*VertexAccessorFunction )( v, PluginRequest::ExamineAccess );
    }
    void discover_vertex( const VDescriptor& v, const Graph& ) {
        ( plugin_->*VertexAccessorFunction )( v, PluginRequest::DiscoverAccess );
    }
    void start_vertex( const VDescriptor& v, const Graph& ) {
        ( plugin_->*VertexAccessorFunction )( v, PluginRequest::StartAccess );
    }
    void finish_vertex( const VDescriptor& v, const Graph& ) {
        ( plugin_->*VertexAccessorFunction )( v, PluginRequest::FinishAccess );
    }

    void examine_edge( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::ExamineAccess );
    }
    void tree_edge( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::TreeEdgeAccess );
    }
    void non_tree_edge( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::NonTreeEdgeAccess );
    }
    void back_edge( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::BackEdgeAccess );
    }
    void gray_target( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::GrayTargetAccess );
    }
    void black_target( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::BlackTargetAccess );
    }
    void forward_or_cross_edge( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::ForwardOrCrossEdgeAccess );
    }
    void edge_relaxed( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::EdgeRelaxedAccess );
    }
    void edge_not_relaxed( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::EdgeNotRelaxedAccess );
    }
    void edge_minimized( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::EdgeMinimizedAccess );
    }
    void edge_not_minimized( const EDescriptor& e, const Graph& ) {
        ( plugin_->*EdgeAccessorFunction )( e, PluginRequest::EdgeNotMinimizedAccess );
    }

protected:
    PluginRequest* plugin_;
};

typedef PluginGraphVisitorHelper< Road::Graph, &PluginRequest::road_vertex_accessor, &PluginRequest::road_edge_accessor > PluginRoadGraphVisitor;
typedef PluginGraphVisitorHelper< PublicTransport::Graph, &PluginRequest::pt_vertex_accessor, &PluginRequest::pt_edge_accessor > PluginPtGraphVisitor;
typedef PluginGraphVisitorHelper< Multimodal::Graph, &PluginRequest::vertex_accessor, &PluginRequest::edge_accessor > PluginGraphVisitor;

}

#endif
