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

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/function_property_map.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include "plugin.hh"
#include "plugin_factory.hh"
#include "utils/timer.hh"
#include "utils/function_property_accessor.hh"

using namespace std;

namespace Tempus {

class WeightDistance
{
public:
    WeightDistance( unsigned traffic_rule ) : traffic_rule_( traffic_rule ) {}
    float operator() ( const Road::Graph& graph, const Road::Edge& e ) {
        if ( ( graph[e].traffic_rules() & traffic_rule_ ) == 0 ) {
            // allowed transport types do not include car
            // It is an oversimplification here : it must depends on allowed transport types selected by the user
            return std::numeric_limits<float>::infinity();
        }
        return graph[e].length();
    }
private:
    int traffic_rule_;
};

class ConstantTravelTime
{
public:
    ConstantTravelTime( const Road::Graph& graph, const float walking_speed, int traffic_rule ) :
        graph_( graph ), walking_speed_( walking_speed * 1000.0 / 60.0 ), traffic_rule_( traffic_rule )
    {}
    float operator() ( const Road::Edge& e ) const {
        if ( ( graph_[e].traffic_rules() & traffic_rule_ ) == 0 ) {
            return std::numeric_limits<float>::infinity();
        }
        return graph_[e].length() / walking_speed_;
    }
private:
    const Road::Graph& graph_;
    const float walking_speed_;
    const int traffic_rule_;
};

class CarTravelTime
{
public:
    CarTravelTime( const Road::Graph& graph ) : graph_( graph ) {}
    float operator() ( const Road::Edge& e ) const {
        if ( (graph_[e].traffic_rules() & TrafficRuleCar) == 0 ) {
            return std::numeric_limits<float>::infinity();
        }
        return graph_[e].length() / (graph_[e].car_speed_limit() * 1000.0) * 60.0;
    }
private:
    const Road::Graph& graph_;
};

class RoadPlugin : public Plugin {
public:
    static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option( "trace_vertex", "Trace vertex traversal", Variant::from_bool(false) );
        odl.declare_option( "prepare_result", "Prepare result", Variant::from_bool(true) );
        return odl;
    }

    static const Capabilities plugin_capabilities() {
        Capabilities params;
        params.optimization_criteria().push_back( CostId::CostDistance );
        params.optimization_criteria().push_back( CostId::CostDuration );
        return params;
    }

    RoadPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "sample_road_plugin", options ) {
        // load graph
        const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
        graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
        if ( graph_ == nullptr ) {
            throw std::runtime_error( "Problem loading the multimodal graph" );
        }
    }

    const RoutingData* routing_data() const { return graph_; }

    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const;

private:
    const Multimodal::Graph* graph_;
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
    RoadPluginRequest( const RoadPlugin* parent, const VariantMap& options, const Multimodal::Graph* graph )
        : PluginRequest( parent, options ), graph_( *graph )
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

    template <typename PredMap, typename DistanceMap, typename WeightPMap>
    void shortest_path( const Road::Graph& road_graph,
                        Road::Vertex origin,
                        PredMap& pred_map,
                        DistanceMap& distance_map,
                        WeightPMap weight_pmap )
    {
        ///
        /// Visitor to be built on 'this'. This way, xxx_accessor methods will be called
        Tempus::PluginRoadGraphVisitor vis( this );

        auto vertex_index = get( boost::vertex_index, road_graph );
        try {
            boost::dijkstra_shortest_paths( road_graph,
                                            origin,
                                            boost::make_iterator_property_map( pred_map.begin(), vertex_index ),
                                            boost::make_iterator_property_map( distance_map.begin(), vertex_index ),
                                            weight_pmap,
                                            vertex_index,
                                            std::less<float>(),
                                            boost::closed_plus<float>(),
                                            std::numeric_limits<float>::max(),
                                            0.0,
                                            vis
                                            );
        }
        catch ( path_found_exception& ) {
            // Dijkstra has been short cut
        }
    }

    virtual std::unique_ptr<Result> process( const Request& request ) {
        REQUIRE( request.allowed_modes().size() == 1 );
        REQUIRE( graph_.road_vertex_from_id( request.origin() ) );
        for ( size_t i = 0; i < request.steps().size(); i++ ) {
            REQUIRE( graph_.road_vertex_from_id(request.steps()[i].location()) );
        }

        db_id_t mode = request.allowed_modes()[0];
        if ( mode != TransportModeWalking && mode != TransportModePrivateBicycle && mode != TransportModePrivateCar ) {
            throw std::runtime_error( "Unsupported transport mode" );
        }

        trace_vertex_  = get_bool_option( "trace_vertex" );
        bool prepare_result = get_bool_option( "prepare_result" );

        iterations_ = 0;

        const Road::Graph& road_graph = graph_.road();

        Timer timer;

        // output predecessor map
        std::vector<Road::Vertex> pred_map( boost::num_vertices( road_graph ) );
        // output distance map

        std::vector<double> distance_map( boost::num_vertices( road_graph ) );
        ///
        /// We define a property map that reads the 'length' member of a Road::Section,
        /// which is the edge property of a Road::Graph
        WeightDistance weight_distance_calculator( graph_.transport_mode( mode )->traffic_rules() );
        auto weight_distance_map = make_function_property_accessor( road_graph, weight_distance_calculator );

        ConstantTravelTime const_tt( road_graph,
                                     mode == TransportModeWalking ? 3.6 : 12.0,
                                     graph_.transport_mode( mode )->traffic_rules() );
        CarTravelTime car_tt( road_graph );
        auto const_weight_map = boost::make_function_property_map<Road::Edge, float, ConstantTravelTime>( const_tt );
        auto car_weight_map = boost::make_function_property_map<Road::Edge, float, CarTravelTime>( car_tt );

        std::list<Road::Vertex> path;
        Road::Vertex origin = graph_.road_vertex_from_id( request.origin() ).get();
        // resolve each step, in reverse order
        bool path_found = true;

        for ( size_t ik = request.steps().size(); ik >= 2; --ik ) {
            size_t i = ik - 1;

            if ( i > 0 ) {
                origin = graph_.road_vertex_from_id(request.steps()[i-1].location()).get();
            }
            else {
                origin = graph_.road_vertex_from_id(request.origin()).get();
            }

            destination_ = graph_.road_vertex_from_id(request.steps()[i].location()).get();

            // call dijkstra
            if ( request.optimizing_criteria()[0] == CostId::CostDistance ) {
                shortest_path( road_graph, origin, pred_map, distance_map, weight_distance_map );
            }
            else if ( mode == TransportModePrivateCar ) {
                shortest_path( road_graph, origin, pred_map, distance_map, car_weight_map );
            }
            else {
                shortest_path( road_graph, origin, pred_map, distance_map, const_weight_map );
            }

            // reorder the path
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

        metrics_[ "time_s" ] = Variant::from_float(timer.elapsed());
        metrics_["iterations"] = Variant::from_int(iterations_);

        if ( !path_found ) {
            throw std::runtime_error( "No path found !" );
        }

        std::unique_ptr<Result> result( new Result() );
        result->push_back( Roadmap() );
        Roadmap& roadmap = result->back().roadmap();

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
            Db::Connection connection( plugin_->db_options() );
            simple_multimodal_roadmap( *result, connection, graph_ );
        }
        return std::move( result );
    }
};

std::unique_ptr<PluginRequest> RoadPlugin::request( const VariantMap& options ) const
{
    return std::unique_ptr<PluginRequest>( new RoadPluginRequest( this, options, graph_ ) );
}

}

DECLARE_TEMPUS_PLUGIN( "sample_road_plugin", Tempus::RoadPlugin )
