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

/**
   Sample plugin that processes very simple user request on a road graph.
   * Only distance minimization is considered

   The plugin finds a route between an origin and a destination via Dijkstra.
 */

#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "plugin_factory.hh"
#include "utils/timer.hh"
#include "utils/function_property_accessor.hh"

using namespace std;

namespace Tempus {

class WeightCalculator
{
public:
    double operator() ( const Road::Graph& graph, const Road::Edge& e ) {
        if ( (graph[e].traffic_rules() & TrafficRulePedestrian) == 0 ) {
            // allowed transport types do not include car
            // It is an oversimplification here : it must depends on allowed transport types selected by the user
            return std::numeric_limits<double>::infinity();
        }
        return graph[e].length();
    }
};

class RoadPlugin : public Plugin {
public:
    static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option( "trace_vertex", "Trace vertex traversal", Variant::fromBool(false) );
        odl.declare_option( "prepare_result", "Prepare result", Variant::fromBool(true) );
        return odl;
    }

    static const Capabilities plugin_capabilities() {
        Capabilities params;
        params.optimization_criteria().push_back( CostId::CostDistance );
        return params;
    }

    RoadPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "sample_road_plugin" ) {
        // load graph
        const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
        graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
        if ( graph_ == nullptr ) {
            throw std::runtime_error( "Problem loading the multimodal graph" );
        }

        schema_name_ = get_option_or_default( options, "db/schema" ).str();
        db_options_ = get_option_or_default( options, "db/options" ).str();
    }

    const RoutingData* routing_data() const { return graph_; }

    virtual std::unique_ptr<PluginRequest> request( const PluginRequest::OptionValueList& options = PluginRequest::OptionValueList() ) const;

private:
    const Multimodal::Graph* graph_;

    std::string db_options_;
    std::string schema_name_;

    friend class RoadPluginRequest;
};

class RoadPluginRequest : public PluginRequest
{
private:
    const Multimodal::Graph& graph_;
    bool trace_vertex_;
    size_t iterations_;
    Road::Vertex destination_;

    struct path_found_exception {};

public:
    RoadPluginRequest( const RoadPlugin* parent, const PluginRequest::OptionValueList& options )
        : PluginRequest( parent, options ), graph_( *parent->graph_ )
    {
    }

    virtual void road_vertex_accessor( const Road::Vertex& v, int access_type ) {
        if ( access_type == PluginRequest::ExamineAccess ) {
            iterations_++;
            if ( v == destination_ ) {
                throw path_found_exception();
            }

            if ( trace_vertex_ ) {
                // very slow
                COUT << "Examining vertex " << graph_.road()[v].db_id() << endl;
            }
        }
    }
    virtual void road_edge_accessor( const Road::Edge& e, int access_type ) {
        if ( access_type == PluginRequest::EdgeRelaxedAccess ) {
            if ( trace_vertex_ ) {
                // very slow
                Road::Vertex u = source( e, graph_.road() );
                Road::Vertex v = target( e, graph_.road() );
                COUT << "Edge relaxed " << graph_.road()[u].db_id() << "->" << graph_.road()[v].db_id() << " w: " << graph_.road()[e].length() << endl;
            }
        }
    }

    virtual std::unique_ptr<Result> process( const Request& request ) {
        REQUIRE( vertex_exists( request.origin(), graph_.road() ) );
        REQUIRE( vertex_exists( request.destination(), graph_.road() ) );

        if ( ( request.optimizing_criteria()[0] != CostId::CostDistance ) ) {
            throw std::invalid_argument( "Unsupported optimizing criterion" );
        }

        bool prepare_result;
        get_option( "trace_vertex", trace_vertex_ );
        get_option( "prepare_result", prepare_result );

        iterations_ = 0;

        const Road::Graph& road_graph = graph_.road();

        Timer timer;

        std::vector<Road::Vertex> pred_map( boost::num_vertices( road_graph ) );
        std::vector<double> distance_map( boost::num_vertices( road_graph ) );
        ///
        /// We define a property map that reads the 'length' (of type double) member of a Road::Section,
        /// which is the edge property of a Road::Graph
        //	    FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, double Road::Section::*> length_map( road_graph, &Road::Section::length );
        WeightCalculator weight_calculator;
        Tempus::FunctionPropertyAccessor<Road::Graph,
                                 boost::edge_property_tag,
                                 double,
                                 WeightCalculator> weight_map( road_graph, weight_calculator );

        ///
        /// Visitor to be built on 'this'. This way, xxx_accessor methods will be called
        Tempus::PluginRoadGraphVisitor vis( this );

        std::list<Road::Vertex> path;
        Road::Vertex origin = request.origin();
        // resolve each step, in reverse order
        bool path_found = true;

        for ( size_t ik = request.steps().size(); ik >= 1; --ik ) {
            size_t i = ik - 1;

            if ( i > 0 ) {
                origin = request.steps()[i-1].location();
            }
            else {
                origin = request.origin();
            }

            destination_ = request.steps()[i].location();

            try {
                boost::dijkstra_shortest_paths( road_graph,
                                                origin,
                                                &pred_map[0],
                                                &distance_map[0],
                                                weight_map,
                                                boost::get( boost::vertex_index, road_graph ),
                                                std::less<double>(),
                                                boost::closed_plus<double>(),
                                                std::numeric_limits<double>::max(),
                                                0.0,
                                                vis
                                              );
            }
            catch ( path_found_exception& ) {
                // Dijkstra has been short cut
            }

            // reorder the path, could have been better included ...
            Road::Vertex current = destination_;

            while ( current != origin ) {
                path.push_front( current );

                if ( pred_map[current] == current ) {
                    path_found = false;
                    break;
                }

                current = pred_map[ current ];
            }

            if ( !path_found ) {
                break;
            }
        }

        path.push_front( origin );

        metrics_[ "time_s" ] = Variant::fromFloat(timer.elapsed());
        metrics_["iterations"] = Variant::fromInt(iterations_);

        if ( !path_found ) {
            throw std::runtime_error( "No path found !" );
        }

        std::unique_ptr<Result> result( new Result() );
        result->push_back( Roadmap() );
        Roadmap& roadmap = result->back();

        roadmap.set_starting_date_time( request.steps()[1].constraint().date_time() );

        std::auto_ptr<Roadmap::Step> step;

        Road::Edge current_road;
        std::list<Road::Vertex>::iterator prev = path.begin();
        std::list<Road::Vertex>::iterator it = prev;
        it++;

        for ( ; it != path.end(); ++it, ++prev) {
            Road::Vertex v = *it;
            Road::Vertex previous = *prev;

            // Find an edge, based on a source and destination vertex
            Road::Edge e;
            bool found = false;
            boost::tie( e, found ) = boost::edge( previous, v, road_graph );

            if ( !found ) {
                continue;
            }

            step.reset( new Roadmap::RoadStep() );
            step->set_cost(CostId::CostDistance, road_graph[e].length());
            step->set_transport_mode(1);
            Roadmap::RoadStep* rstep = static_cast<Roadmap::RoadStep*>(step.get());
            rstep->set_road_edge_id( road_graph[e].db_id() );
            roadmap.add_step( step );
        }

        if ( prepare_result ) {
            Db::Connection connection( static_cast<const RoadPlugin*>(plugin_)->db_options_ );
            simple_multimodal_roadmap( *result, connection, graph_ );
        }
        return std::move( result );
    }
};

std::unique_ptr<PluginRequest> RoadPlugin::request( const PluginRequest::OptionValueList& options ) const
{
    return std::unique_ptr<PluginRequest>( new RoadPluginRequest( this, options ) );
}

}

DECLARE_TEMPUS_PLUGIN( "sample_road_plugin", Tempus::RoadPlugin )
