#include "ch_plugin.hh"

#include <boost/heap/d_ary_heap.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/format.hpp>

#include "utils/associative_property_map_default_value.hh"
#include "utils/timer.hh"

#include "ch_query_graph.hh"

namespace Tempus {

const CHPlugin::OptionDescriptionList CHPlugin::option_descriptions()
{
    Plugin::OptionDescriptionList odl;
    return odl;
}

const CHPlugin::PluginCapabilities CHPlugin::plugin_capabilities()
{
    Plugin::PluginCapabilities caps;
    caps.optimization_criteria().push_back( CostId::CostDuration );
    return caps;
}

class CHEdgeProperty
{
public:
    uint32_t cost        :31;
    uint32_t is_shortcut :1;
    db_id_t db_id;
};

using CHQuery = CHQueryGraph<CHEdgeProperty>;

using CHVertex = uint32_t;
using CHEdge = CHQueryGraph<CHEdgeProperty>::edge_descriptor;

using MiddleNodeMap = std::map<std::pair<CHVertex, CHVertex>, CHVertex>;

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


struct SPriv
{
    // middle node of a contraction
    MiddleNodeMap middle_node;

    std::unique_ptr<CHQuery> ch_query;

    // node index -> node id
    std::vector<db_id_t> node_id;
};

void* CHPlugin::spriv_ = nullptr;

void CHPlugin::post_build()
{
    if ( spriv_ == nullptr ) {
        spriv_ = new SPriv;
    }
    SPriv* spriv = (SPriv*)spriv_;

    std::cout << "loading CH graph ..." << std::endl;
    Db::Connection conn( Application::instance()->db_options() );

    size_t num_nodes = 0;
    {
        Db::Result r( conn.exec( "select count(*) from ch.ordered_nodes" ) );
        BOOST_ASSERT( r.size() == 1 );
        r[0][0] >> num_nodes;
    }
    {
        Db::ResultIterator res_it = conn.exec_it( "select * from\n"
                                                  "(\n"
                                                  // the upward part
                                                  "select o1.id as id1, o2.id as id2, weight, o3.id as mid, 0 as dir, rs1.id as eid1, rs2.id as eid2, o1.node_id, o2.node_id, o3.node_id\n"
                                                  "from ch.query_graph\n"
                                                  "left join ch.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  "left join tempus.road_section as rs1 on rs1.node_from = node_inf and rs1.node_to = node_sup\n"
                                                  "left join tempus.road_section as rs2 on rs2.node_from = node_sup and rs2.node_to = node_inf\n"
                                                  ", ch.ordered_nodes as o1, ch.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 1 > 0\n"

                                                  // union with the downward part
                                                  "union all\n"
                                                  "select o1.id as id1, o2.id as id2, weight, o3.id as mid, 1 as dir, rs1.id as eid1, rs2.id as eid2, o1.node_id, o2.node_id, o3.node_id\n"
                                                  "from ch.query_graph\n"
                                                  "left join ch.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  "left join tempus.road_section as rs1 on rs1.node_from = node_inf and rs1.node_to = node_sup\n"
                                                  "left join tempus.road_section as rs2 on rs2.node_from = node_sup and rs2.node_to = node_inf\n"
                                                  ", ch.ordered_nodes as o1, ch.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 2 > 0\n"
                                                  ") t order by id1, dir, id2, weight asc"
                                                  );
        Db::ResultIterator it_end;

        std::vector<std::pair<uint32_t,uint32_t>> targets;
        std::vector<CHEdgeProperty> properties;
        std::vector<uint16_t> up_degrees;

        uint32_t node = 0;
        uint16_t upd = 0;
        uint32_t old_id1 = 0;
        uint32_t old_id2 = 0;
        int old_dir = 0;
        for ( ; res_it != it_end; res_it++ ) {
            Db::RowValue res_i = *res_it;
            uint32_t id1 = res_i[0].as<uint32_t>() - 1;
            uint32_t id2 = res_i[1].as<uint32_t>() - 1;
            int dir = res_i[4];
            if ( (id1 == old_id1) && (id2 == old_id2) && (dir == old_dir) ) {
                // we may have the same edges with different costs
                // we then skip the duplicates and only take
                // the first one (the one with the smallest weight)
                continue;
            }
            old_id1 = id1;
            old_id2 = id2;
            old_dir = dir;

            db_id_t vid1, vid2;
            res_i[7] >> vid1;
            res_i[8] >> vid2;

            db_id_t eid = 0;
            if ( !res_i[5].is_null() ) {
                res_i[5] >> eid;
            }
            else if ( !res_i[6].is_null() ) {
                res_i[6] >> eid;
            }
            if ( id1 > node ) {
                up_degrees.push_back( upd );
                node = id1;
                upd = 0;
            }
            if ( dir == 0 ) {
                upd++;
            }
            CHEdgeProperty p;
            p.cost = res_i[2];
            p.db_id = eid;
            p.is_shortcut = 0;
            if ( !res_i[3].is_null() ) {
                uint32_t middle = res_i[3].as<uint32_t>() - 1;
                // we have a middle node, it is a shortcut
                p.is_shortcut = 1;
                if ( dir == 0 ) {
                    spriv->middle_node[std::make_pair(id1, id2)] = middle;
                }
                else {
                    spriv->middle_node[std::make_pair(id2, id1)] = middle;
                }
            }
            //std::cout << std::endl;
            properties.emplace_back( p );
            targets.emplace_back( std::make_pair(id1, id2) );
        }
        up_degrees.push_back( upd );

        spriv->ch_query.reset( new CHQueryGraph<CHEdgeProperty>( targets.begin(), num_nodes, up_degrees.begin(), properties.begin() ) );
        std::cout << "OK" << std::endl;
    }
    {
        spriv->node_id.resize( num_nodes );
        Db::ResultIterator res_it = conn.exec_it( "select id, node_id from ch.ordered_nodes" );
        Db::ResultIterator it_end;
        for ( ; res_it != it_end; res_it++ ) {
            Db::RowValue res_i = *res_it;
            CHVertex v = res_i[0].as<uint32_t>() - 1;
            db_id_t id = res_i[1];
            BOOST_ASSERT( v < spriv->node_id.size() );
            spriv->node_id[v] = id;
        }
    }

    // check consistency
    {
        CHQuery& ch = *spriv->ch_query;
        for ( CHVertex v = 0; v < num_vertices(ch); v++ ) {
            for ( auto oeit = out_edges( v, ch ).first; oeit != out_edges( v, ch ).second; oeit++ ) {
                BOOST_ASSERT( target( *oeit, ch ) > v );
            }
            for ( auto ieit = in_edges( v, ch ).first; ieit != in_edges( v, ch ).second; ieit++ ) {
                BOOST_ASSERT( source( *ieit, ch ) > v );
            }
        }
    }
    {
        CHQuery& ch = *spriv->ch_query;
        for ( auto it = edges( ch ).first; it != edges( ch ).second; it++ ) {
            CHVertex u = source( *it, ch );
            CHVertex v = target( *it, ch );
            bool found = false;
            CHEdge e;
            boost::tie( e, found ) = edge( u, v, ch );
            BOOST_ASSERT( found );
            CHVertex u2 = source( e, ch );
            CHVertex v2 = target( e, ch );
            BOOST_ASSERT( u == u2 );
            BOOST_ASSERT( v == v2 );
            BOOST_ASSERT( it->property().cost == e.property().cost );
        }
    }
}

template <typename Graph, typename Vertex, typename CostType, typename WeightMap, typename NodeId>
std::list<Vertex> bidirectional_ch_dijkstra( const Graph& graph, Vertex origin, Vertex destination, WeightMap weight_map, CostType& ret_cost, const NodeId& /*node_id*/ )
{
    std::map<Vertex, CostType> costs;
    std::list<Vertex> returned_path;

    const CostType infinity = std::numeric_limits<CostType>::max();

    typedef std::vector<CostType> PotentialMap;
    size_t n = num_vertices( graph );
    PotentialMap potential[2] = { PotentialMap( n, infinity ),
                                  PotentialMap( n, infinity ) };
    typedef CostType* PotentialPMap;
    PotentialPMap potential_map[2] = { &potential[0][0], &potential[1][0] };

    typedef boost::indirect_cmp<PotentialPMap, std::greater<CostType>> Cmp;

    typedef boost::heap::d_ary_heap< Vertex, boost::heap::arity<4>, boost::heap::compare< Cmp >, boost::heap::mutable_<true> > VertexQueue;
    Cmp cmp_fw( potential_map[0] );
    Cmp cmp_bw( potential_map[1] );
    VertexQueue vertex_queue[2] = { VertexQueue( cmp_fw ), VertexQueue( cmp_bw ) };

    std::map<Vertex, Vertex> predecessor[2];

    vertex_queue[0].push( origin );
    put( potential_map[0], origin, 0.0 );
    vertex_queue[1].push( destination );
    put( potential_map[1], destination, 0.0 );

    // direction : 0 = forward, 1 = backward
    int dir = 1;

    Vertex top_node = 0;
    CostType total_cost = infinity;
    bool path_found = false;

    auto get_min_pi = [&vertex_queue,&potential_map]( int dir ) {
        if ( !vertex_queue[dir].empty() ) {
            return get( potential_map[dir], vertex_queue[dir].top() );
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

        Vertex min_v = vertex_queue[dir].top();
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
                Vertex vv = target( *oei, graph );

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
                Vertex vv = source( *iei, graph );

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

    std::list<Vertex> path;
    Vertex x = top_node;
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
    Vertex t = top_node;
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

std::pair<std::list<CHVertex>, float> ch_query( SPriv* spriv, CHVertex ch_origin, CHVertex ch_destination )
{
    std::pair<std::list<CHVertex>, float> ret;

    auto weight_map_fn = []( const CHQuery::edge_descriptor& e ) {
        return float(e.property().cost / 100.0);
    };
    auto weight_map = boost::make_function_property_map<CHQuery::edge_descriptor, float, decltype(weight_map_fn)>( weight_map_fn );
    float ret_cost = std::numeric_limits<float>::max();
    auto path = bidirectional_ch_dijkstra( *spriv->ch_query, ch_origin, ch_destination, weight_map, ret_cost, spriv->node_id );

    auto& ret_path = ret.first;
    unpack_path( path.begin(), path.end(), spriv->middle_node, std::back_inserter( ret_path ) );
    ret.second = ret_cost;

    return ret;
}

void CHPlugin::pre_process( Request& request )
{
    REQUIRE( vertex_exists( request.origin(), graph_.road() ) );
    REQUIRE( vertex_exists( request.destination(), graph_.road() ) );

    request_ = request;

    result_.clear();
}

void CHPlugin::process()
{
    Timer timer;

    const Road::Graph& graph = graph_.road();

    SPriv* spriv = (SPriv*)spriv_;
    CHVertex ch_origin, ch_destination;

    db_id_t origin_id = graph[request_.origin()].db_id();
    db_id_t destination_id = graph[request_.destination()].db_id();

    bool origin_found = false;
    bool destination_found = false;
    for ( size_t i = 0; i < spriv->node_id.size(); i++ ) {
        if ( spriv->node_id[i] == origin_id ) {
            ch_origin = i;
            origin_found = true;
        }
        else if ( spriv->node_id[i] == destination_id ) {
            ch_destination = i;
            destination_found = true;
        }
    }

    if ( !origin_found ) {
        throw std::runtime_error( (boost::format("Can't find vertex of ID %1") % origin_id).str() );
    }
    if ( !destination_found ) {
        throw std::runtime_error( (boost::format("Can't find vertex of ID %1") % destination_id).str() );
    }
    std::cout << "From " << origin_id << " to " << destination_id << std::endl;

    auto ch_ret = ch_query( spriv, ch_origin, ch_destination );
    auto& ch_graph = *spriv->ch_query;

    auto& path = ch_ret.first;

    if ( path.empty() ) {
        throw std::runtime_error( "No path found !" );
    }

    metrics_[ "time_s" ] = Variant::fromFloat( timer.elapsed() );

    result_.push_back( Roadmap() );
    Roadmap& roadmap = result_.back();

    roadmap.set_starting_date_time( request_.steps()[1].constraint().date_time() );

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
            std::cout << "Cannot find edge (" << spriv->node_id[previous] << "->" << spriv->node_id[v] << ")" << std::endl;
        }
        BOOST_ASSERT( found );

        step.reset( new Roadmap::RoadStep() );
        step->set_cost( CostId::CostDistance, e.property().cost / 100.0 );
        step->set_transport_mode(1);
        Roadmap::RoadStep* rstep = static_cast<Roadmap::RoadStep*>(step.get());
        rstep->set_road_edge_id( e.property().db_id );
        roadmap.add_step( step );
    }
}

} // namespace Tempus

DECLARE_TEMPUS_PLUGIN( "ch_plugin", Tempus::CHPlugin )

