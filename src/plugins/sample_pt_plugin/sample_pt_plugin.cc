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

#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"
#include "utils/graph_db_link.hh"
#include "utils/function_property_accessor.hh"

using namespace std;

namespace Tempus {


class LengthCalculator {
public:
    LengthCalculator( Db::Connection& db ) : db_( db ) {}

    // TODO - possible improvement: cache length
    double operator()( const PublicTransport::Graph& graph, const PublicTransport::Edge& e ) {
        Db::Result res = db_.exec( ( boost::format( "SELECT ST_Length(geom) FROM tempus.pt_section WHERE stop_from = %1% AND stop_to = %2%" )
                                     % get_stop_from(graph[e]).db_id() % get_stop_to(graph[e]).db_id() ).str() );
        BOOST_ASSERT( res.size() > 0 );
        double l = res[0][0].as<double>();
        return l;
    }
protected:
    Db::Connection& db_;
};

class PtPlugin : public Plugin {
public:

    static const OptionDescriptionList option_descriptions() {
        OptionDescriptionList odl;
        odl.declare_option( "origin_pt_stop", "Origin stop node", 0 );
        odl.declare_option( "destination_pt_stop", "Destination stop node", 0 );
        return odl;
    }

    static const PluginCapabilities plugin_capabilities() {
        PluginCapabilities params;
        params.optimization_criteria().push_back( CostDistance );
        params.optimization_criteria().push_back( CostDuration );
        params.set_depart_after( true );
        params.set_arrive_before( false );
        return params;
    }

    PtPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~PtPlugin() {
    }

public:
    virtual void pre_process( Request& request ) {
        REQUIRE( graph_.public_transports().size() >= 1 );
        REQUIRE( request.steps().size() == 2 );

        if ( ( request.optimizing_criteria()[0] != CostDuration ) && ( request.optimizing_criteria()[0] != CostDistance ) ) {
            throw std::invalid_argument( "Unsupported optimizing criterion" );
        }

        request_ = request;
        result_.clear();
    }

    virtual void pt_vertex_accessor( const PublicTransport::Vertex& v, int access_type ) {
        if ( access_type == Plugin::ExamineAccess ) {
            const PublicTransport::Graph& pt_graph = *graph_.public_transports().begin()->second;
            CERR << "Examining vertex " << pt_graph[v].db_id() << endl;
        }
    }
    virtual void process() {
        const PublicTransport::Graph& pt_graph = *graph_.public_transports().begin()->second;
        db_id_t network_id = graph_.public_transports().begin()->first;

        PublicTransport::Vertex departure, arrival;

        // if stops are given by the corresponding options, get them
        int departure_id, arrival_id;
        get_option( "origin_pt_stop", departure_id );
        get_option( "destination_pt_stop", arrival_id );

        if ( departure_id != 0 && arrival_id != 0 ) {
            CERR << "departure id " << departure_id << " arrival id " << arrival_id << std::endl;
            bool found_v;
            boost::tie( departure,found_v ) = vertex_from_id( departure_id, pt_graph );

            if ( ! found_v ) {
                throw std::runtime_error( "Cannot find departure ID" );
            }

            boost::tie( arrival,found_v ) = vertex_from_id( arrival_id, pt_graph );

            if ( ! found_v ) {
                throw std::runtime_error( "Cannot find arrival ID" );
            }
        }
        else {
            // else use regular road nodes from the request
            const Road::Graph& road_graph = graph_.road();
            CERR << "origin = " << road_graph[request_.origin()].db_id() << " dest = " << road_graph[request_.destination()].db_id() << endl;

            // for each step in the request, find the corresponding public transport node
            for ( size_t i = 0; i < 2; i++ ) {
                Road::Vertex node;

                if ( i == 0 ) {
                    node = request_.origin();
                }
                else {
                    node = request_.destination();
                }

                PublicTransport::Vertex found_vertex;

                std::string q = ( boost::format( "select s.id from tempus.road_node as n join tempus.pt_stop as s on st_dwithin( n.geom, s.geom, 100 ) "
                                                 "where n.id = %1% order by st_distance( n.geom, s.geom) asc limit 1" ) % road_graph[node].db_id() ).str();
                Db::Result res = db_.exec( q );

                if ( res.size() < 1 ) {
                    throw std::runtime_error( ( boost::format( "Cannot find node %1%" ) % node ).str() );
                }

                db_id_t vid = res[0][0].as<db_id_t>();
                found_vertex = vertex_from_id( vid, pt_graph ).first;
                {
                    if ( i == 0 ) {
                        departure = found_vertex;
                    }

                    if ( i == 1 ) {
                        arrival = found_vertex;
                    }

                    CERR << "Road node #" << node << " <-> Public transport node " << pt_graph[found_vertex].db_id() << std::endl;
                }
            }
        }

        CERR << "departure = " << departure << " arrival = " << arrival << endl;

        //
        // Call to Dijkstra
        //

        std::vector<PublicTransport::Vertex> pred_map( boost::num_vertices( pt_graph ) );
        std::vector<double> distance_map( boost::num_vertices( pt_graph ) );

        LengthCalculator length_calculator( db_ );
        FunctionPropertyAccessor<PublicTransport::Graph,
                                 boost::edge_property_tag,
                                 double,
                                 LengthCalculator> length_map( pt_graph, length_calculator );

        PluginPtGraphVisitor vis( this );
        boost::dijkstra_shortest_paths( pt_graph,
                                        departure,
                                        &pred_map[0],
                                        &distance_map[0],
                                        length_map,
                                        boost::get( boost::vertex_index, pt_graph ),
                                        std::less<double>(),
                                        boost::closed_plus<double>(),
                                        std::numeric_limits<double>::max(),
                                        0.0,
                                        vis
                                      );

        // reorder the path
        std::list<PublicTransport::Vertex> path;
        {
            PublicTransport::Vertex current = arrival;
            bool found = true;

            while ( current != departure ) {
                path.push_front( current );

                if ( pred_map[current] == current ) {
                    found = false;
                    break;
                }

                current = pred_map[ current ];
            }

            if ( !found ) {
                throw std::runtime_error( "No path found" );
            }

            path.push_front( departure );
        }

        //
        // Final result building.
        //

        // we result in only one roadmap
        result_.push_back( Roadmap() );
        Roadmap& roadmap = result_.back();

        roadmap.set_starting_date_time( request_.steps()[0].constraint().date_time() );

        bool first_loop = true;

        Road::Vertex previous = *path.begin();

        for ( std::list<PublicTransport::Vertex>::iterator it = path.begin(); it != path.end(); it++ ) {
            CERR << "path " << pt_graph[ *it ].db_id() << std::endl;

            if ( first_loop ) {
                first_loop = false;
                continue;
            }

            Roadmap::PublicTransportStep* ptstep = new Roadmap::PublicTransportStep();

            ptstep->set_trip_id(1);
            ptstep->set_departure_stop( previous );
            ptstep->set_arrival_stop( *it );
            ptstep->set_network_id( network_id );
            ptstep->set_transport_mode( 1 );
            ptstep->set_cost( CostDistance, distance_map[*it] );

            previous = *it;

            roadmap.add_step( std::auto_ptr<Roadmap::Step>(ptstep) );
        }
    }

    void cleanup() {
        // nothing special to clean up
    }

    static void post_build() {}

};
}
DECLARE_TEMPUS_PLUGIN( "sample_pt_plugin", Tempus::PtPlugin )



