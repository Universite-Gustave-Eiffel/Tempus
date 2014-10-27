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
#include "pgsql_importer.hh"
#include "utils/timer.hh"
#include "utils/function_property_accessor.hh"

using namespace std;

namespace Tempus {

    class WeightCalculator
    {
    public:
        double operator() ( const Road::Graph& graph, const Road::Edge& e ) {
            if ( (graph[e].traffic_rules() & TrafficRuleCar) == 0 ) {
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
        odl.declare_option( "trace_vertex", "Trace vertex traversal", false );
        odl.declare_option( "prepare_result", "Prepare result", true );
        return odl;
    }

    static const PluginCapabilities plugin_capabilities() {
        PluginCapabilities params;
        params.optimization_criteria().push_back( CostDistance );
        return params;
    }

    RoadPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~RoadPlugin() {
    }
protected:
    bool trace_vertex_;
    bool prepare_result_;

    // exception returned to shortcut Dijkstra
    struct path_found_exception {};

    Road::Vertex destination_;

public:
    static void post_build() { }

    virtual void pre_process( Request& request ) {
        REQUIRE( vertex_exists( request.origin(), graph_.road() ) );
        REQUIRE( vertex_exists( request.destination(), graph_.road() ) );

        if ( ( request.optimizing_criteria()[0] != CostDistance ) ) {
            throw std::invalid_argument( "Unsupported optimizing criterion" );
        }

        request_ = request;

        get_option( "trace_vertex", trace_vertex_ );
        get_option( "prepare_result", prepare_result_ );

        result_.clear();
    }

    virtual void road_vertex_accessor( const Road::Vertex& v, int access_type ) {
        if ( access_type == Plugin::ExamineAccess ) {
            if ( v == destination_ ) {
                throw path_found_exception();
            }

            if ( trace_vertex_ ) {
                // very slow
                COUT << "Examining vertex " << v << endl;
            }
        }
    }

    virtual void process() {
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
        Road::Vertex origin = request_.origin();
        // resolve each step, in reverse order
        bool path_found = true;

        for ( size_t ik = request_.steps().size(); ik >= 1; --ik ) {
            size_t i = ik - 1;

            if ( i > 0 ) {
                origin = request_.steps()[i-1].location();
            }
            else {
                origin = request_.origin();
            }

            destination_ = request_.steps()[i].location();

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

        metrics_[ "time_s" ] = timer.elapsed();

        if ( !path_found ) {
            throw std::runtime_error( "No path found !" );
        }

        result_.push_back( Roadmap() );
        Roadmap& roadmap = result_.back();

        roadmap.set_starting_date_time( request_.steps()[0].constraint().date_time() );

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
            step->set_cost(CostDistance, road_graph[e].length());
            step->set_transport_mode(1);
            Roadmap::RoadStep* rstep = static_cast<Roadmap::RoadStep*>(step.get());
            rstep->set_road_edge( e );
            roadmap.add_step( step );
        }
    }

    void cleanup() {
        // nothing special to clean up
    }

    Result& result() {
        if ( prepare_result_ ) {
            return Plugin::result();
        }

        return result_;
    }
};
}
DECLARE_TEMPUS_PLUGIN( "sample_road_plugin", Tempus::RoadPlugin )



