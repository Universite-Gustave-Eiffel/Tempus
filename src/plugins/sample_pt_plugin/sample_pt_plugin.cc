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
#include "plugin_factory.hh"
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
                                     % get_stop_from(graph, e).db_id() % get_stop_to(graph, e).db_id() ).str() );
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
        odl.declare_option( "origin_pt_stop", "Origin stop node", Variant::from_int(0) );
        odl.declare_option( "destination_pt_stop", "Destination stop node", Variant::from_int(0) );
        return odl;
    }

    static const Capabilities plugin_capabilities() {
        Capabilities params;
        params.optimization_criteria().push_back( CostId::CostDistance );
        params.optimization_criteria().push_back( CostId::CostDuration );
        params.set_depart_after( true );
        params.set_arrive_before( false );
        return params;
    }

    PtPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "sample_pt_plugin", options ) {
        // load graph
        const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
        graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
        if ( graph_ == nullptr ) {
            throw std::runtime_error( "Problem loading the multimodal graph" );
        }
    }

    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const;

    const RoutingData* routing_data() const { return graph_; }

private:
    const Multimodal::Graph* graph_;
};

class PtPluginRequest : public PluginRequest
{
private:
    const Multimodal::Graph& graph_;

public:
    PtPluginRequest( const PtPlugin* parent, const VariantMap& options, const Multimodal::Graph* graph )
        : PluginRequest( parent, options ), graph_( *graph )
    {
    }

    virtual void pt_vertex_accessor( const PublicTransport::Vertex& v, int access_type ) {
        if ( access_type == PluginRequest::ExamineAccess ) {
            const PublicTransport::Graph& pt_graph = *(*graph_.public_transports().begin()).second;
            CERR << "Examining vertex " << pt_graph[v].db_id() << endl;
        }
    }


    virtual std::unique_ptr<Result> process( const Request& request ) {
        REQUIRE( graph_.public_transports().size() >= 1 );
        REQUIRE( request.steps().size() == 2 );

        if ( ( request.optimizing_criteria()[0] != CostId::CostDuration ) && ( request.optimizing_criteria()[0] != CostId::CostDistance ) ) {
            throw std::invalid_argument( "Unsupported optimizing criterion" );
        }

        Db::Connection db( plugin_->db_options() );

        auto p = *graph_.public_transports().begin();
        const PublicTransport::Graph& pt_graph = *p.second;
        db_id_t network_id = p.first;

        PublicTransport::Vertex departure = 0, arrival = 0;

        // if stops are given by the corresponding options, get them
        db_id_t departure_id = get_int_option( "origin_pt_stop" );
        db_id_t arrival_id = get_int_option( "destination_pt_stop" );

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
            CERR << "origin = " << request.origin() << " dest = " << request.destination() << endl;

            // for each step in the request, find the corresponding public transport node
            for ( size_t i = 0; i < 2; i++ ) {
                db_id_t node;

                if ( i == 0 ) {
                    node = request.origin();
                }
                else {
                    node = request.destination();
                }

                PublicTransport::Vertex found_vertex;

                std::string q = ( boost::format( "select s.id from tempus.road_node as n join tempus.pt_stop as s on st_dwithin( n.geom, s.geom, 100 ) "
                                                 "where n.id = %1% order by st_distance( n.geom, s.geom) asc limit 1" ) % node ).str();
                Db::Result res = db.exec( q );

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

        LengthCalculator length_calculator( db );
        FunctionPropertyAccessor<PublicTransport::Graph,
                                 boost::edge_property_tag,
                                 double,
                                 LengthCalculator> length_map( pt_graph, length_calculator );

        PluginPtGraphVisitor vis( this );
        boost::dijkstra_shortest_paths( pt_graph,
                                        departure,
                                        boost::make_iterator_property_map( pred_map.begin(), get( boost::vertex_index, pt_graph ) ),
                                        boost::make_iterator_property_map( distance_map.begin(), get( boost::vertex_index, pt_graph ) ),
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
        std::unique_ptr<Result> result( new Result() );
        result->push_back( Roadmap() );
        Roadmap& roadmap = result->back();

        roadmap.set_starting_date_time( request.steps()[1].constraint().date_time() );

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
            ptstep->set_departure_stop( pt_graph[previous].db_id() );
            ptstep->set_arrival_stop( pt_graph[*it].db_id() );
            ptstep->set_network_id( network_id );
            ptstep->set_transport_mode( 1 );
            ptstep->set_cost( CostId::CostDistance, distance_map[*it] );

            previous = *it;

            roadmap.add_step( std::auto_ptr<Roadmap::Step>(ptstep) );
        }

        return std::move( result );
    }
};

std::unique_ptr<PluginRequest> PtPlugin::request( const VariantMap& options ) const
{
    return std::unique_ptr<PluginRequest>( new PtPluginRequest( this, options, graph_ ) );
}

}

DECLARE_TEMPUS_PLUGIN( "sample_pt_plugin", Tempus::PtPlugin )


