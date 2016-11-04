/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *   Copyright (C) 2015 Mappy <dt.lbs.route@mappy.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ch_plugin.hh"
#include "plugin_factory.hh"

//#include <boost/heap/d_ary_heap.hpp>
#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <queue>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/format.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include "utils/associative_property_map_default_value.hh"
#include "utils/timer.hh"

#include "utils/graph_db_link.hh"

namespace Tempus {

const Plugin::OptionDescriptionList CHPlugin::option_descriptions()
{
    Plugin::OptionDescriptionList odl;
    return odl;
}

const Plugin::Capabilities CHPlugin::plugin_capabilities()
{
    Plugin::Capabilities caps;
    caps.optimization_criteria().push_back( CostId::CostDuration );
    return caps;
}

CHPlugin::CHPlugin( ProgressionCallback& progression, const VariantMap& options ) : Plugin( "ch_plugin", options )
{
    // load graph
    const RoutingData* rd = load_routing_data( "ch_graph", progression, options );
    rd_ = dynamic_cast<const CHRoutingData*>( rd );
    if ( rd_ == nullptr ) {
        throw std::runtime_error( "Problem loading the CH routing data" );
    }
}



template <typename OutIterator>
void unpack_edge( CHVertex v1, CHVertex v2, const MiddleNodeMap& middle_node, OutIterator out_it )
{
    auto it = middle_node.find( std::make_pair(v1,v2) );
    if ( it != middle_node.end() ) {
        unpack_edge( v1, it->second, middle_node, out_it );
        unpack_edge( it->second, v2, middle_node, out_it );
    }
    else {
        *out_it = v1;
        out_it++;
    }
}

template <typename InIterator, typename OutIterator>
void unpack_path( InIterator it_begin, InIterator it_end, const MiddleNodeMap& middle_node, OutIterator out_it )
{
    if ( it_begin == it_end )
        return;
    auto it = it_begin;
    auto it2 = it_begin;
    it2++;
    for ( ; it2 != it_end; it++, it2++ ) {
        unpack_edge( *it, *it2, middle_node, out_it );
    }
    *out_it = *it;
    out_it++;
}


template <typename CostType, typename WeightMap>
std::list<CHVertex> bidirectional_ch_dijkstra( const CHRoutingData& rd, CHVertex origin, CHVertex destination, WeightMap weight_map, CostType& ret_cost )
{
    const CHQuery& graph = rd.ch_query();

    std::map<CHVertex, CostType> costs;
    std::list<CHVertex> returned_path;

    const CostType infinity = std::numeric_limits<CostType>::max();

    typedef std::vector<CostType> PotentialMap;
    size_t n = num_vertices( graph );
    PotentialMap potential[2] = { PotentialMap( n, infinity ),
                                  PotentialMap( n, infinity ) };
    typedef CostType* PotentialPMap;
    PotentialPMap potential_map[2] = { &potential[0][0], &potential[1][0] };

    typedef boost::indirect_cmp<PotentialPMap, std::greater<CostType>> Cmp;

    // FIXME is the heap needed ? since the graph is partitioned in two acyclic graphs with a topological order on nodes
    // There may be a way to be faster: loop over each node in order and relax out edges
    //typedef boost::heap::d_ary_heap< CHVertex, boost::heap::arity<4>, boost::heap::compare< Cmp >, boost::heap::mutable_<true> > VertexQueue;
    typedef std::priority_queue<CHVertex, std::vector<CHVertex>, Cmp> VertexQueue;
    Cmp cmp_fw( potential_map[0] );
    Cmp cmp_bw( potential_map[1] );
    VertexQueue vertex_queue[2] = { VertexQueue( cmp_fw ), VertexQueue( cmp_bw ) };

    std::map<CHVertex, CHVertex> predecessor[2];

    vertex_queue[0].push( origin );
    put( potential_map[0], origin, 0.0 );
    vertex_queue[1].push( destination );
    put( potential_map[1], destination, 0.0 );

    // direction : 0 = forward, 1 = backward
    int dir = 1;

    CHVertex top_node = 0;
    CostType total_cost = infinity;
    bool path_found = false;

    auto get_min_pi = [&vertex_queue,&potential_map]( int ldir ) {
        if ( !vertex_queue[ldir].empty() ) {
            return get( potential_map[ldir], vertex_queue[ldir].top() );
        }
        return std::numeric_limits<CostType>::max();
    };

    while ( !vertex_queue[0].empty() || !vertex_queue[1].empty() ) {

        if ( std::min( get_min_pi(dir), get_min_pi(1-dir) ) > total_cost ) {
            // we've reached the best path
            break;
        }

        // interleave directions
        dir = 1 - dir;

        if ( vertex_queue[dir].empty() )
            dir = 1 - dir;

        CHVertex min_v = vertex_queue[dir].top();
        vertex_queue[dir].pop();
        CostType min_pi = get( potential_map[dir], min_v );

        {
            CostType min_pi2 = get( potential_map[1-dir], min_v );
            // if min_pi2 is not infinity, it means this node has already been seen
            // in the other direction
            // so it is a candidate top node
            if ( min_pi + min_pi2 < total_cost ) {
                top_node = min_v;
                total_cost = min_pi + min_pi2;
                path_found = true;
            }
        }

        if ( dir == 0 ) {
            for ( auto oei = out_edges( min_v, graph ).first;
                  oei != out_edges( min_v, graph ).second;
                  oei++ ) {
                CHVertex vv = target( *oei, graph );

                CostType new_pi = get( potential_map[dir], vv );
                CostType cost = get( weight_map, *oei );
                if ( min_pi + cost < new_pi ) {
                    // relax edge
                    put( potential_map[dir], vv, min_pi + cost );
                    vertex_queue[dir].push( vv );
                    predecessor[dir][vv] = min_v;
                }
            }
        }
        else {
            for ( auto iei = in_edges( min_v, graph ).first;
                  iei != in_edges( min_v, graph ).second;
                  iei++ ) {
                CHVertex vv = source( *iei, graph );

                CostType new_pi = get( potential_map[dir], vv );
                CostType cost = get( weight_map, *iei );
                if ( min_pi + cost < new_pi ) {
                    // relax edge
                    put( potential_map[dir], vv, min_pi + cost );
                    vertex_queue[dir].push( vv );
                    predecessor[dir][vv] = min_v;
                }
            }
        }
    }

    if ( !path_found ) {
        //std::cout << "NO PATH FOUND" << std::endl;
        return returned_path;
    }
    //std::cout << "PATH FOUND" << std::endl;

    // path from origin (s) to top node (x)
    // s = p[p[p[p[p[...[x]]]]]] , ..., p[x], x
    //std::cout << "top node " << node_id[top_node] << std::endl;

    std::list<CHVertex> path;
    CHVertex x = top_node;
    while (x != origin)
    {
        returned_path.push_front( x );
        auto it = predecessor[0].find( x );
        BOOST_ASSERT_MSG( it != predecessor[0].end(), "Can't find upward predecessor" );
        x = it->second;
    }
    returned_path.push_front( origin );

    // path from destination (t) to top node (x)
    // t = p[p[p[p[p[...[x]]]]]] , ..., p[x], x
    CHVertex t = top_node;
    while ( t != destination )
    {
        auto it = predecessor[1].find( t );
        BOOST_ASSERT_MSG( it != predecessor[0].end(), "Can't find downward predecessor" );
        t = it->second;
        returned_path.push_back( t );
    }

    ret_cost = total_cost;
    return returned_path;
}

std::pair<std::list<CHVertex>, float> ch_query( const CHRoutingData& rd, CHVertex ch_origin, CHVertex ch_destination )
{
    std::pair<std::list<CHVertex>, float> ret;

    auto weight_map_fn = []( const CHQuery::edge_descriptor& e ) {
        return float(e.property().b.cost / 100.0);
    };
    auto weight_map = boost::make_function_property_map<CHQuery::edge_descriptor, float, decltype(weight_map_fn)>( weight_map_fn );
    float ret_cost = std::numeric_limits<float>::max();
    auto path = bidirectional_ch_dijkstra( rd, ch_origin, ch_destination, weight_map, ret_cost );

    auto& ret_path = ret.first;
    unpack_path( path.begin(), path.end(), rd.middle_node(), std::back_inserter( ret_path ) );
    ret.second = ret_cost;

    return ret;
}

class CHPluginRequest : public PluginRequest
{
private:
    const CHRoutingData& rd_;
public:
    CHPluginRequest( const CHPlugin* parent, const VariantMap& options, const CHRoutingData& rd )
        : PluginRequest( parent, options), rd_(rd)
    {}

    std::unique_ptr<Result> process( const Request& request ) override
    {
        Timer timer;

        boost::optional<CHVertex> origin = rd_.vertex_from_id( request.origin() );
        boost::optional<CHVertex> destination = rd_.vertex_from_id( request.destination() );

        if ( !origin ) {
            throw std::runtime_error( (boost::format("Can't find vertex of ID %1%") % request.origin()).str() );
        }
        if ( !destination ) {
            throw std::runtime_error( (boost::format("Can't find vertex of ID %1%") % request.destination()).str() );
        }
        std::cout << "From " << request.origin() << " to " << request.destination() << std::endl;

        auto ch_ret = ch_query( rd_, origin.get(), destination.get() );
        auto& ch_graph = rd_.ch_query();

        auto& path = ch_ret.first;

        if ( path.empty() ) {
            throw std::runtime_error( "No path found !" );
        }

        metrics_[ "time_s" ] = Variant::from_float( timer.elapsed() );

        std::unique_ptr<Result> result( new Result() );
        result->push_back( Roadmap() );
        Roadmap& roadmap = result->back().roadmap();

        roadmap.set_starting_date_time( request.steps()[1].constraint().date_time() );

        std::auto_ptr<Roadmap::Step> step;

        Road::Edge current_road;
        auto prev = path.begin();
        auto it = prev;
        it++;

        for ( ; it != path.end(); ++it, ++prev) {
            CHVertex v = *it;
            CHVertex previous = *prev;

            // Find an edge, based on a source and destination vertex
            CHEdge e;
            bool found = false;
            boost::tie( e, found ) = edge( previous, v, ch_graph );

            if ( !found ) {
                std::cout << "Cannot find edge (" << rd_.vertex_id( previous ) << "->" << rd_.vertex_id( v ) << ")" << std::endl;
            }
            BOOST_ASSERT( found );

            step.reset( new Roadmap::RoadStep() );
            step->set_cost( CostId::CostDistance, e.property().b.cost / 100.0 );
            step->set_transport_mode(1);
            Roadmap::RoadStep* rstep = static_cast<Roadmap::RoadStep*>(step.get());
            rstep->set_road_edge_id( e.property().db_id );
            roadmap.add_step( step );
        }

        Db::Connection connection( plugin_->db_options() );
        fill_roadmap_from_db( roadmap.begin(), roadmap.end(), connection );
        return result;
    }
};


std::unique_ptr<PluginRequest> CHPlugin::request( const VariantMap& options ) const
{
    return std::unique_ptr<PluginRequest>( new CHPluginRequest( this, options, *rd_ ) );
}

} // namespace Tempus

DECLARE_TEMPUS_PLUGIN( "ch_plugin", Tempus::CHPlugin )

