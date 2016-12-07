/**
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

#include "utils/struct_vector_member_property_map.hh"

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/format.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/thread/thread.hpp>
#include <boost/heap/d_ary_heap.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include "plugin.hh"
#include "plugin_factory.hh"
#include "utils/timer.hh"
#include "utils/function_property_accessor.hh"

using namespace std;

namespace Tempus {

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

class AStarRoadPlugin : public Plugin {
public:
    static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option( "prepare_result", "Prepare result", Variant::from_bool(true) );
        odl.declare_option( "AStar/speed_heuristic", "Max speed (km/h) to use in A* heuristic", Variant::from_float( 90.0 ) );
        odl.declare_option( "Time/walking_speed", "Average walking speed (km/h)", Variant::from_float( 3.6 ));
        odl.declare_option( "Time/cycling_speed", "Average cycling speed (km/h)", Variant::from_int( 12.0 ));
        return odl;
    }

    static const Capabilities plugin_capabilities() {
        Capabilities params;
        params.optimization_criteria().push_back( CostId::CostDistance );
        params.optimization_criteria().push_back( CostId::CostDuration );
        return params;
    }

    AStarRoadPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "astar_road_plugin", options ) {
        // load graph
        const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
        graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
        if ( graph_ == nullptr ) {
            throw std::runtime_error( "Problem loading the multimodal graph" );
        }
    }

    const RoutingData* routing_data() const { return graph_; }

    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const;

    struct VertexRoutingData
    {
        float distance;
        float cost;
        Road::Vertex pred;
        boost::default_color_type color;

        VertexRoutingData() :
            distance( std::numeric_limits<float>::max() )
            , cost ( std::numeric_limits<float>::max() )
            , pred( 0 )
            , color( boost::white_color ) {
        }

        VertexRoutingData( Road::Vertex v ) :
            distance( std::numeric_limits<float>::max() )
            , cost ( std::numeric_limits<float>::max() )
            , pred( v )
            , color( boost::white_color ) {
        }
    };

    using ThreadRoutingData = std::map<boost::thread::id, std::vector<VertexRoutingData> >;

    ThreadRoutingData& thread_routing_data() const { return thread_routing_data_; }

private:
    const Multimodal::Graph* graph_;

    mutable ThreadRoutingData thread_routing_data_;
};


struct astar_goal_found {};

// Visitor used to shortcut A* when at destination
class GoalVisitor : public boost::default_astar_visitor
{
public:
    GoalVisitor( Road::Vertex goal, size_t& iterations ) : goal_( goal ), iterations_( iterations ) {}
    void examine_vertex( Road::Vertex u, const Road::Graph& ) {
        if( u == goal_ )
            throw astar_goal_found();
        iterations_++;
    }
private:
    Road::Vertex goal_;
    size_t& iterations_;
};

struct EuclidianHeuristic
{
public:
    EuclidianHeuristic( const Road::Graph& g, Road::Vertex v, double max_speed ) :
        graph_( g ), max_speed_( max_speed * 1000.0 / 60.0 )
    {
        Point3D p( g[v].coordinates() );
        destination_ = Point2D( p.x(), p.y() );
    }

    float operator()( const Road::Vertex& v )
    {
        return distance( destination_, Point2D( graph_[v].coordinates().x(), graph_[v].coordinates().y() ) ) / max_speed_;
    }
private:
    const Road::Graph& graph_;
    Point2D destination_;
    // in m/min
    const float max_speed_;
};

class AStarRoadPluginRequest : public PluginRequest
{
private:
    const Multimodal::Graph& graph_;

    struct path_found_exception {};

public:
    AStarRoadPluginRequest( const AStarRoadPlugin* parent, const VariantMap& options, const Multimodal::Graph* graph )
        : PluginRequest( parent, options ), graph_( *graph )
    {
    }

    virtual std::unique_ptr<Result> process( const Request& request ) {
        std::cerr << "origin=" << request.origin() << std::endl;
        std::cerr << "destination=" << request.destination() << std::endl;
        REQUIRE( graph_.road_vertex_from_id( request.origin() ) );
        REQUIRE( graph_.road_vertex_from_id( request.destination() ) );
        if ( request.allowed_modes().size() > 1 ) {
            throw std::runtime_error( "This is a monomodal plugin" );
        }
        db_id_t mode = request.allowed_modes()[0];
        if ( mode != TransportModeWalking && mode != TransportModePrivateBicycle && mode != TransportModePrivateCar ) {
            throw std::runtime_error( "Unsupported transport mode" );
        }

        bool prepare_result = get_bool_option( "prepare_result" );
        double max_speed = get_float_option( "AStar/speed_heuristic" );
        double walking_speed = get_float_option( "Time/walking_speed" );
        double cycling_speed = get_float_option( "Time/cycling_speed" );

        const Road::Graph& road_graph = graph_.road();

        Timer timer;

        metrics_[ "time_alloc" ] = Variant::from_float( timer.elapsed() );

        size_t nv = num_vertices( road_graph );

        AStarRoadPlugin* p = static_cast< AStarRoadPlugin* >( const_cast<Plugin*>(  plugin_ ) );
        auto& vertex_data = p->thread_routing_data()[ boost::this_thread::get_id() ];
        for ( size_t i = 0; i < nv; i++ ) {
            vertex_data[i] = AStarRoadPlugin::VertexRoutingData( i );
        }

        metrics_[ "time_init" ] = Variant::from_float( timer.elapsed() );

        ConstantTravelTime const_tt( road_graph,
                                     mode == TransportModeWalking ? walking_speed : cycling_speed,
                                     mode == TransportModeWalking ? TrafficRulePedestrian : TrafficRuleBicycle );
        CarTravelTime car_tt( road_graph );
        auto const_weight_map = boost::make_function_property_map<Road::Edge, float, ConstantTravelTime>( const_tt );
        auto car_weight_map = boost::make_function_property_map<Road::Edge, float, CarTravelTime>( car_tt );

        std::list<Road::Vertex> path;
        Road::Vertex origin = graph_.road_vertex_from_id( request.origin() ).get();
        Road::Vertex destination = graph_.road_vertex_from_id( request.destination() ).get();

        std::cout << "origin " << origin << " destination " << destination << std::endl;

        size_t iterations = 0;
        GoalVisitor vis( destination, iterations );

        EuclidianHeuristic h( road_graph, destination, max_speed );

        auto pred_map = make_struct_vector_member_property_map( vertex_data, &AStarRoadPlugin::VertexRoutingData::pred );
        auto cost_map = make_struct_vector_member_property_map( vertex_data, &AStarRoadPlugin::VertexRoutingData::cost );
        auto distance_map = make_struct_vector_member_property_map( vertex_data, &AStarRoadPlugin::VertexRoutingData::distance );
        auto color_map = make_struct_vector_member_property_map( vertex_data, &AStarRoadPlugin::VertexRoutingData::color );

        distance_map[origin] = 0.0;
        cost_map[origin] = h( origin );

        put( color_map, origin, boost::white_color );

        bool path_found = true;

        auto vertex_index = get( boost::vertex_index, road_graph );
        try {
            if ( mode == TransportModePrivateCar ) {
                astar_search_no_init( road_graph,
                                      origin,
                                      h,
                                      vis,
                                      pred_map,
                                      cost_map,
                                      distance_map,
                                      car_weight_map,
                                      color_map,
                                      vertex_index,
                                      std::less<float>(),
                                      boost::closed_plus<float>(),
                                      std::numeric_limits<float>::max(),
                                      0.0 );
            }
            else {
                astar_search( road_graph,
                              origin,
                              h,
                              vis,
                              pred_map,
                              cost_map,
                              distance_map,
                              const_weight_map,
                              vertex_index,
                              color_map,
                              std::less<float>(),
                              boost::closed_plus<float>(),
                              std::numeric_limits<float>::max(),
                              0.0 );
            }
        }
        catch ( astar_goal_found& ) {
            // short cut
        }

        // reorder the path
        Road::Vertex current = destination;

        while ( current != origin ) {
            path.push_front( current );

            if ( pred_map[current] == current ) {
                path_found = false;
                break;
            }

            current = pred_map[ current ];
        }

        path.push_front( origin );

        metrics_[ "time_s" ] = Variant::from_float( timer.elapsed() );
        metrics_["iterations"] = Variant::from_int( iterations );

        if ( !path_found ) {
            throw std::runtime_error( "No path found !" );
        }

        std::unique_ptr<Result> result( new Result() );
        result->push_back( Roadmap() );
        Roadmap& roadmap = result->back().roadmap();
        roadmap.set_starting_date_time( request.steps()[1].constraint().date_time() );

        if ( prepare_result ) {
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
                if ( mode == TransportModePrivateCar ) {
                    step->set_cost(CostId::CostDuration, get( car_weight_map, e ) );
                }
                else {
                    step->set_cost(CostId::CostDuration, get( const_weight_map, e ) );
                }
                step->set_transport_mode(1);
                Roadmap::RoadStep* rstep = static_cast<Roadmap::RoadStep*>(step.get());
                rstep->set_road_edge_id( road_graph[e].db_id() );
                roadmap.add_step( step );
            }

            Db::Connection connection( plugin_->db_options() );
            simple_multimodal_roadmap( *result, connection, graph_ );
        }

        return result;
    }
};

std::unique_ptr<PluginRequest> AStarRoadPlugin::request( const VariantMap& options ) const
{
    auto it = thread_routing_data().find( boost::this_thread::get_id() );
    if ( it == thread_routing_data().end() ) {
        size_t nv = num_vertices( graph_->road() );
        // allocate
        std::cout << "allocate for thread " << boost::this_thread::get_id() << " " << nv << " elements" << std::endl;
        thread_routing_data()[boost::this_thread::get_id()] = std::vector<AStarRoadPlugin::VertexRoutingData>( nv );
    }
    return std::unique_ptr<PluginRequest>( new AStarRoadPluginRequest( this, options, graph_ ) );
}

}

DECLARE_TEMPUS_PLUGIN( "astar_road_plugin", Tempus::AStarRoadPlugin )
