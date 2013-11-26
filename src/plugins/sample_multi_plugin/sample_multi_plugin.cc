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

#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"

using namespace std;

namespace Tempus {
// length (in meters) of each section
static std::map< Multimodal::Edge, double > distances;
// duration of each section (for an average pedestrian)
static std::map< Multimodal::Edge, double > durations;

class MultiPlugin : public Plugin {
public:

    static const OptionDescriptionList option_descriptions() {
        return OptionDescriptionList();
    }

    MultiPlugin( const std::string& nname, const std::string db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~MultiPlugin() {
    }

private:

public:

    ///
    /// In the post_build, we pre-compute a table of distances for each edge
    ///
    static void post_build() {
        // retrieve the length of each pt section
        std::cout << "Computing the length of each section..." << std::endl;
        typedef std::map< std::pair<db_id_t, db_id_t>, double > PtLength;
        PtLength pt_lengths;

        Db::Connection db( Application::instance()->db_options() );
        Db::Result res = db.exec( "SELECT stop_from, stop_to, ST_Length(geom) FROM tempus.pt_section" );

        for ( size_t i = 0; i < res.size(); ++i ) {
            db_id_t stop_from, stop_to;
            double length = 0.0;
            res[i][0] >> stop_from;
            res[i][1] >> stop_to;
            res[i][2] >> length;

            pt_lengths[std::make_pair( stop_from, stop_to )] = length;
        }

        REQUIRE( pt_lengths.size() == res.size() );

        Multimodal::EdgeIterator ei, ei_end;

        for ( boost::tie( ei, ei_end ) = edges( Application::instance()->graph() ); ei != ei_end; ei++ ) {
            switch ( ei->connection_type() ) {
            case Multimodal::Edge::Road2Road: {
                Road::Edge e;
                bool found;
                boost::tie( e, found ) = road_edge( *ei );
                distances[*ei] = Application::instance()->graph().road[e].length;
                // average pedestrian walk : 5000 m / h
                durations[*ei] = distances[*ei] / 5000.0 * 60.0;
            }
            break;

            case Multimodal::Edge::Transport2Transport: {
                PublicTransport::Edge e;
                const PublicTransport::Graph& pt_graph = *( ei->source.pt_graph );
                bool found;
                boost::tie( e, found ) = public_transport_edge( *ei );

                if ( !found ) {
                    CERR << "Can't find pt edge" << *ei << std::endl;
                }

                distances[*ei] = pt_lengths[ std::make_pair( pt_graph[e].stop_from, pt_graph[e].stop_to ) ];
                // average public transport speed : 25 km / h
                durations[*ei] = distances[*ei] / 25000.0 * 60.0;
            }
            break;

            case Multimodal::Edge::Road2Transport: {
                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( ei->target.pt_graph );
                const Road::Graph& road_graph = *( ei->source.road_graph );
                double abscissa = pt_graph[ei->target.pt_vertex].abscissa_road_section;
                Road::Edge e = pt_graph[ ei->target.pt_vertex ].road_section;

                // if we are coming from the start point of the road
                if ( source( e, road_graph ) == ei->source.road_vertex ) {
                    distances[*ei] = road_graph[e].length * abscissa;
                }
                // otherwise, that is the opposite direction
                else {
                    distances[*ei] = road_graph[e].length * ( 1 - abscissa );
                }

                // average walking speed + average waiting time on the stop (say 5 minutes )
                durations[*ei] = distances[*ei] / 5000.0 * 60.0 + 5.0;
            }
            break;

            case Multimodal::Edge::Transport2Road: {
                // find the road section where the stop is attached to
                const PublicTransport::Graph& pt_graph = *( ei->source.pt_graph );
                const Road::Graph& road_graph = *( ei->target.road_graph );
                double abscissa = pt_graph[ei->source.pt_vertex].abscissa_road_section;
                Road::Edge e = pt_graph[ ei->source.pt_vertex ].road_section;

                // if we are coming from the start point of the road
                if ( source( e, road_graph ) == ei->target.road_vertex ) {
                    distances[*ei] = road_graph[e].length * abscissa;
                }
                // otherwise, that is the opposite direction
                else {
                    distances[*ei] = road_graph[e].length * ( 1 - abscissa );
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

        REQUIRE( distances.size() == num_edges( Application::instance()->graph() ) );
    }

    virtual void pre_process( Request& request ) {
        request_ = request;

        for ( size_t i = 0; i < request.optimizing_criteria.size(); ++i ) {
            REQUIRE( request.optimizing_criteria[i] == CostDistance || request.optimizing_criteria[i] == CostDuration );
        }

        result_.clear();
    }

    Multimodal::Vertex vertex_from_road_node_id( db_id_t id ) {
        Multimodal::VertexIterator vi, vi_end;

        for ( boost::tie( vi, vi_end ) = vertices( graph_ ); vi != vi_end; vi++ ) {
            if ( vi->type == Multimodal::Vertex::Road && graph_.road[ vi->road_vertex ].db_id == id ) {
                return *vi;
            }
        }

        throw std::runtime_error( "bug: should not reach here" );
    }

    ///
    /// A path is a list of vertices
    typedef std::list<Multimodal::Vertex> Path;

    ///
    /// Convert a path into a roadmap
    void add_roadmap( const Path& path ) {
        result_.push_back( Roadmap() );
        Roadmap& roadmap = result_.back();

        roadmap.total_costs[CostDistance] = 0.0;
        roadmap.total_costs[CostDuration] = 0.0;
        std::list<Multimodal::Vertex>::const_iterator previous = path.begin();
        COUT << "first: " << *previous << std::endl;
        std::list<Multimodal::Vertex>::const_iterator it = ++previous;
        --previous;

        for ( ; it != path.end(); ++it ) {
            Multimodal::Edge me;
            {
                bool found;
                boost::tie( me, found ) = edge( *previous, *it, graph_ );
            }

            Roadmap::Step* mstep = 0;

            if ( previous->type == Multimodal::Vertex::Road && it->type == Multimodal::Vertex::Road ) {
                mstep = new Roadmap::RoadStep();
                Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( mstep );
                Road::Edge e;
                bool found = false;
                boost::tie( e, found ) = edge( previous->road_vertex, it->road_vertex, *it->road_graph );

                if ( !found ) {
                    throw std::runtime_error( "Can't find the road edge !" );
                }

                step->road_section = e;
                step->vertex_from = previous->road_vertex;
            }

            else if ( previous->type == Multimodal::Vertex::PublicTransport && it->type == Multimodal::Vertex::PublicTransport ) {
                mstep = new Roadmap::PublicTransportStep();
                Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( mstep );
                PublicTransport::Edge e;
                bool found = false;
                boost::tie( e, found ) = edge( previous->pt_vertex, it->pt_vertex, *it->pt_graph );

                if ( !found ) {
                    throw std::runtime_error( "Can't find the pt edge !" );
                }

                step->section = e;
                // Set the trip ID
                step->trip_id = 1;

                // find the network_id
                for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports.begin(); nit != graph_.public_transports.end(); ++nit ) {
                    if ( it->pt_graph == &nit->second ) {
                        step->network_id = nit->first;
                        break;
                    }
                }
            }
            else {
                // Make a multimodal edge and copy it into the roadmap as a 'generic' step
                mstep = new Roadmap::GenericStep( Multimodal::Edge( *previous, *it ) );
            }

            roadmap.steps.push_back( mstep );
            mstep->costs[CostDistance] = distances[me];
            mstep->costs[CostDuration] = durations[me];
            roadmap.total_costs[CostDistance] += distances[me];
            roadmap.total_costs[CostDuration] += durations[me];

            previous = it;
        }
    }

    // the exception to throw to short cut dijkstra
    struct PathFound {};

    // current destination
    Multimodal::Vertex destination_;

    void vertex_accessor( Multimodal::Vertex v, int access_type ) {
        if ( access_type == Plugin::ExamineAccess ) {
            if ( v == destination_ ) {
                throw PathFound();
            }
        }
    }

    ///
    /// Try to find the shortest path between origin and destination, optimizing the passed criterion
    /// The given path is not cleared
    /// The bool returned is false if no path has been found
    bool find_path( const Multimodal::Vertex& origin, const Multimodal::Vertex& destination, int optimizing_criterion, Path& path ) {
        size_t n = num_vertices( graph_ );
        std::vector<boost::default_color_type> color_map( n );
        std::vector<Multimodal::Vertex> pred_map( n );
        // distance map (in number of nodes)
        std::vector<double> node_distance_map( n );

        Multimodal::VertexIndexProperty vertex_index = get( boost::vertex_index, graph_ );

        Tempus::PluginGraphVisitor vis( this );
        destination_ = destination;

        try {
            if ( optimizing_criterion == CostDistance ) {
                boost::dijkstra_shortest_paths( graph_,
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
            else if ( optimizing_criterion == CostDuration ) {
                boost::dijkstra_shortest_paths( graph_,
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

        path.push_front( origin );
        return true;
    }

    ///
    /// The main process
    virtual void process() {
        //
        // Here we run the optimization for each criterion (this is NOT a multi-objective optimisation)
        for ( size_t i = 0; i < request_.optimizing_criteria.size(); ++i ) {
            //
            // Run for each intermiadry steps

            Path path;

            Multimodal::Vertex vorigin, vdestination;
            vorigin = Multimodal::Vertex( &graph_.road, request_.origin );

            for ( size_t j = 0; j < request_.steps.size(); ++j ) {
                if ( j > 0 ) {
                    vorigin = Multimodal::Vertex( &graph_.road, request_.steps[j - 1].destination );
                }

                vdestination = Multimodal::Vertex( &graph_.road, request_.steps[j].destination );
                COUT << "Resolving from " << vorigin << " to " << vdestination << std::endl;

                bool found;
                found = find_path( vorigin, vdestination, request_.optimizing_criteria[i], path );

                if ( !found ) {
                    std::stringstream err;
                    err << "Cannot find a path between " << vorigin << " and " << vdestination;
                    throw std::runtime_error( err.str() );
                }
            }

            // convert the path to a roadmap
            add_roadmap( path );
        }
    }

    void cleanup() {
        // nothing special to clean up
    }

};
}
DECLARE_TEMPUS_PLUGIN( "sample_multi_plugin", Tempus::MultiPlugin )
