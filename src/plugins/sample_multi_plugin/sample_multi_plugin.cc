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
   Sample plugin that deals with multimodal path planning.
   The chosen database must have at least one public transport network.

   This plugin is able to process shortest (CostDistance) and fastest (CostDuration) queries.

   The graph algorithm is very simple (Dijkstra). Intermediary steps are processed independently.

   It also supports multiple results, but not multi-objective requests.
   If the request contains multiple criteria, a result is added for each criterion.
 */

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include "plugin.hh"
#include "plugin_factory.hh"
#include "db.hh"
#include "utils/timer.hh"

using namespace std;

namespace Tempus {
// length (in meters) of each section
static std::map< Multimodal::Edge, double > distances;
// duration of each section (for an average pedestrian)
static std::map< Multimodal::Edge, double > durations;

class MultiPlugin : public Plugin {
public:

    static Plugin::OptionDescriptionList option_descriptions()
    {
        return Plugin::common_option_descriptions();
    }

    static Plugin::Capabilities plugin_capabilities()
    {
        Plugin::Capabilities caps;
        caps.optimization_criteria().push_back( CostId::CostDistance );
        caps.optimization_criteria().push_back( CostId::CostDuration );
        caps.set_intermediate_steps( true );
        caps.set_depart_after( true );
        caps.set_arrive_before( false );
        return caps;
    }


    MultiPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "sample_multi_plugin", options )
    {
        // load graph
        const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
        graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
        if ( graph_ == nullptr ) {
            throw std::runtime_error( "Problem loading the multimodal graph" );
        }

        ///
        /// In the post_build, we pre-compute a table of distances for each edge
        ///

        // retrieve the length of each pt section
        std::cout << "Computing the length of each section..." << std::endl;
        typedef std::map< std::pair<db_id_t, db_id_t>, double > PtLength;
        PtLength pt_lengths;

        Db::Connection db( db_options() );
        Db::Result res = db.exec( "SELECT stop_from, stop_to, ST_Length(geom) FROM tempus.pt_section" );

        for ( size_t i = 0; i < res.size(); ++i ) {
            db_id_t stop_from = 0, stop_to = 0;
            double length = 0.0;
            res[i][0] >> stop_from;
            res[i][1] >> stop_to;
            res[i][2] >> length;

            pt_lengths[std::make_pair( stop_from, stop_to )] = length;
        }

        REQUIRE( pt_lengths.size() == res.size() );

        Multimodal::EdgeIterator ei, ei_end;

        const Road::Graph& road_graph = graph_->road();

        for ( boost::tie( ei, ei_end ) = edges( *graph_ ); ei != ei_end; ei++ ) {
            switch ( ei->connection_type() ) {
            case Multimodal::Edge::Road2Road: {
                Road::Edge e = ei->road_edge();
                distances[*ei] = road_graph[e].length();
                // average pedestrian walk : 5000 m / h
                durations[*ei] = distances[*ei] / 5000.0 * 60.0;
            }
            break;

            case Multimodal::Edge::Transport2Transport: {
                PublicTransport::Edge e;
                const PublicTransport::Graph& pt_graph = *( ei->source().pt_graph() );
                bool found;
                boost::tie( e, found ) = public_transport_edge( *ei );

                if ( !found ) {
                    CERR << "Can't find pt edge" << *ei << std::endl;
                }

                distances[*ei] = pt_lengths[ std::make_pair( get_stop_from(pt_graph, e).db_id(), get_stop_to(pt_graph, e).db_id() ) ];
                // average public transport speed : 25 km / h
                durations[*ei] = distances[*ei] / 25000.0 * 60.0;
            }
            break;

            case Multimodal::Edge::Road2Transport: {
                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( ei->target().pt_graph() );
                double abscissa = pt_graph[ei->target().pt_vertex()].abscissa_road_section();
                Road::Edge e = pt_graph[ ei->target().pt_vertex() ].road_edge();

                // if we are coming from the start point of the road
                if ( source( e, road_graph ) == ei->source().road_vertex() ) {
                    distances[*ei] = road_graph[e].length() * abscissa;
                }
                // otherwise, that is the opposite direction
                else {
                    distances[*ei] = road_graph[e].length() * ( 1 - abscissa );
                }

                // average walking speed + average waiting time on the stop (say 5 minutes )
                durations[*ei] = distances[*ei] / 5000.0 * 60.0 + 5.0;
            }
            break;

            case Multimodal::Edge::Transport2Road: {
                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( ei->source().pt_graph() );
                double abscissa = pt_graph[ei->source().pt_vertex()].abscissa_road_section();
                Road::Edge e = pt_graph[ ei->source().pt_vertex() ].road_edge();

                // if we are coming from the start point of the road
                if ( source( e, road_graph ) == ei->target().road_vertex() ) {
                    distances[*ei] = road_graph[e].length() * abscissa;
                }
                // otherwise, that is the opposite direction
                else {
                    distances[*ei] = road_graph[e].length() * ( 1 - abscissa );
                }

                // average walking speed
                durations[*ei] = distances[*ei] / 5000.0;
            }
            break;

            default: {
                // No POI involved
                distances[*ei] = std::numeric_limits<double>::max();
                durations[*ei] = std::numeric_limits<double>::max();
            }
            }
        }

        REQUIRE( distances.size() == num_edges( *graph_ ) );
    }


    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const;

    const RoutingData* routing_data() const { return graph_; }

private:
    const Multimodal::Graph* graph_;
};

class MultiPluginRequest : public PluginRequest
{
private:
    const Multimodal::Graph* graph_;

public:
    MultiPluginRequest( const MultiPlugin* parent, const VariantMap& options, const Multimodal::Graph* graph ) : PluginRequest( parent, options ), graph_(graph)
    {
    }

public:
    ///
    /// A path is a list of vertices
    typedef std::list<Multimodal::Vertex> Path;

    ///
    /// Convert a path into a roadmap
    void add_roadmap( const Request& request, Result& result, const Path& path ) {
        result.push_back( Roadmap() );
        Roadmap& roadmap = result.back();

        roadmap.set_starting_date_time( request.steps()[1].constraint().date_time() );

        std::list<Multimodal::Vertex>::const_iterator previous = path.begin();
        std::list<Multimodal::Vertex>::const_iterator it = ++previous;
        --previous;

        for ( ; it != path.end(); ++it, ++previous ) {
            Roadmap::Step* mstep = 0;

            if ( previous->type() == Multimodal::Vertex::Road && it->type() == Multimodal::Vertex::Road ) {
                mstep = new Roadmap::RoadStep();
                Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( mstep );
                Road::Edge e;
                bool found = false;
                boost::tie( e, found ) = edge( previous->road_vertex(), it->road_vertex(), *it->road_graph() );

                if ( !found ) {
                    throw std::runtime_error( "Can't find the road edge !" );
                }

                step->set_road_edge_id(graph_->road()[e].db_id());
            }

            else if ( previous->type() == Multimodal::Vertex::PublicTransport && it->type() == Multimodal::Vertex::PublicTransport ) {
                mstep = new Roadmap::PublicTransportStep();
                Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( mstep );
                step->set_departure_stop( get_mm_vertex(*previous).id() );
                step->set_arrival_stop( get_mm_vertex(*it).id() );
                // Set the trip ID
                step->set_trip_id( 1 );

                // find the network_id
                for ( auto p : graph_->public_transports() ) {
                    if ( it->pt_graph() == p.second ) {
                        step->set_network_id( p.first );
                        break;
                    }
                }
            }
            else {
                // Make a multimodal edge and copy it into the roadmap as a 'generic' step
                mstep = new Roadmap::TransferStep( get_mm_vertex( *previous ), get_mm_vertex( *it ) );
            }

            // build the multimodal edge to find corresponding costs
            // we don't use edge() since it will loop over the whole graph each time
            // we assume the edge exists in these maps
            Multimodal::Edge me( *graph_, *previous, *it );
            mstep->set_cost(CostId::CostDistance, distances[me]);
            mstep->set_cost(CostId::CostDuration, durations[me]);
            mstep->set_transport_mode( TransportModeWalking );

            roadmap.add_step( std::auto_ptr<Roadmap::Step>(mstep) );
        }
    }

    // the exception to throw to short cut dijkstra
    struct PathFound {};

    // current destination
    Multimodal::Vertex destination_;

    int iterations_;

    void vertex_accessor( const Multimodal::Vertex& v, int access_type ) {
        if ( access_type == PluginRequest::ExamineAccess ) {
            iterations_++;
            if ( v == destination_ ) {
                throw PathFound();
            }
        }
    }

    ///
    /// Try to find the shortest path between origin and destination, optimizing the passed criterion
    /// The given path is not cleared
    /// The bool returned is false if no path has been found
    bool find_path( const Multimodal::Vertex& origin, const Multimodal::Vertex& destination, CostId optimizing_criterion, Path& path ) {
        size_t n = num_vertices( *graph_ );
        std::vector<boost::default_color_type> color_map( n );
        std::vector<Multimodal::Vertex> pred_map( n );
        // distance map (in number of nodes)
        std::vector<double> node_distance_map( n );

        Multimodal::VertexIndexProperty vertex_index = get( boost::vertex_index, *graph_ );

        Tempus::PluginGraphVisitor vis( this );
        destination_ = destination;
        try {
            if ( optimizing_criterion == CostId::CostDistance ) {
                boost::dijkstra_shortest_paths( *graph_,
                                                origin,
                                                boost::make_iterator_property_map( pred_map.begin(), vertex_index ),
                                                boost::make_iterator_property_map( node_distance_map.begin(), vertex_index ),
                                                boost::make_assoc_property_map( distances ),
                                                vertex_index,
                                                std::less<double>(),
                                                boost::closed_plus<double>(),
                                                std::numeric_limits<double>::max(),
                                                0.0,
                                                vis,
                                                boost::make_iterator_property_map( color_map.begin(), vertex_index )
                                                );
            }
            else if ( optimizing_criterion == CostId::CostDuration ) {
                boost::dijkstra_shortest_paths( *graph_,
                                                origin,
                                                boost::make_iterator_property_map( pred_map.begin(), vertex_index ),
                                                boost::make_iterator_property_map( node_distance_map.begin(), vertex_index ),
                                                boost::make_assoc_property_map( durations ),
                                                vertex_index,
                                                std::less<double>(),
                                                boost::closed_plus<double>(),
                                                std::numeric_limits<double>::max(),
                                                0.0,
                                                vis,
                                                boost::make_iterator_property_map( color_map.begin(), vertex_index )
                                                );
            }
        }
        catch ( PathFound& ) {
            // Do nothing, dijkstra has just been aborted
        }

        COUT << "Dijkstra OK" << endl;

        Multimodal::Vertex current = destination;
        bool found = true;

        while ( current != origin ) {
            path.push_front( current );
            Multimodal::Vertex ncurrent = pred_map[ vertex_index[ current ] ];

            if ( ncurrent == current ) {
                found = false;
                break;
            }

            current = ncurrent;
        }

        if ( !found ) {
            return false;
        }

        return true;
    }

    ///
    /// The main process
    virtual std::unique_ptr<Result> process( const Request& request )
    {
        std::unique_ptr<Result> result( new Result() );
        for ( size_t i = 0; i < request.optimizing_criteria().size(); ++i ) {
            REQUIRE( request.optimizing_criteria()[i] == CostId::CostDistance || request.optimizing_criteria()[i] == CostId::CostDuration );
        }

        iterations_ = 0;

        const Multimodal::Graph& graph = *graph_;

        //
        // Here we run the optimization for each criterion (this is NOT a multi-objective optimisation)
        for ( size_t i = 0; i < request.optimizing_criteria().size(); ++i ) {
            //
            // Run for each intermiadry steps

            Multimodal::Vertex vorigin, vdestination;
            vorigin = Multimodal::Vertex( graph, graph.road_vertex_from_id(request.origin()).get(), Multimodal::Vertex::road_t() );

            Timer timer;

            // global path
            Path path;
            for ( size_t j = 1; j < request.steps().size(); ++j ) {
                // path of this step
                Path lpath;

                vorigin = Multimodal::Vertex( graph, graph.road_vertex_from_id(request.steps()[j - 1].location()).get(), Multimodal::Vertex::road_t() );
                vdestination = Multimodal::Vertex( graph, graph.road_vertex_from_id(request.steps()[j].location()).get(), Multimodal::Vertex::road_t() );

                bool found;
                found = find_path( vorigin, vdestination, request.optimizing_criteria()[i], lpath );

                metrics_[ "time_s" ] = Variant::from_float(timer.elapsed());

                if ( !found ) {
                    std::stringstream err;
                    err << "Cannot find a path between " << vorigin << " and " << vdestination;
                    throw std::runtime_error( err.str() );
                }

                // copy step path to global path
                std::copy( lpath.begin(), lpath.end(), std::back_inserter(path) );
            }
            // add origin back
            path.push_front( Multimodal::Vertex( graph, graph.road_vertex_from_id(request.origin()).get(), Multimodal::Vertex::road_t() ) );

            metrics_[ "time_s" ] = Variant::from_float(timer.elapsed());
            metrics_["iterations"] = Variant::from_int(iterations_);

            // convert the path to a roadmap
            add_roadmap( request, *result, path );
        }
        Db::Connection connection( plugin_->db_options() );
        simple_multimodal_roadmap( *result, connection, graph );

        return std::move(result);
    }
};

std::unique_ptr<PluginRequest> MultiPlugin::request( const VariantMap& options ) const
{
    return std::unique_ptr<PluginRequest>( new MultiPluginRequest( this, options, graph_ ) );
}


}
DECLARE_TEMPUS_PLUGIN( "sample_multi_plugin", Tempus::MultiPlugin )
