#include "ch_plugin.hh"

#include <boost/heap/d_ary_heap.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/property_map/function_property_map.hpp>

#include "utils/associative_property_map_default_value.hh"

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
};

typedef CHQueryGraph<CHEdgeProperty> CHQuery;

typedef uint32_t CHVertex;

typedef std::map<std::pair<CHVertex, CHVertex>, CHVertex> MiddleNodeMap;

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

    std::vector<db_id_t> node_id;
};

void* CHPlugin::spriv_ = nullptr;

void CHPlugin::post_build()
{
    if ( spriv_ == nullptr ) {
        spriv_ = new SPriv;
    }
    SPriv* spriv = (SPriv*)spriv_;
#if 0

    const Road::Graph& road_graph = Application::instance()->graph()->road();

    CHGraph& ch_graph = spriv->ch_graph;
    
    // vertex sorted by order => vertex
    std::map<CHVertex,CHVertex> node_map;
    // copy graph;
    for ( size_t v = 0; v < num_vertices( road_graph ); v++ ) {
        add_vertex( ch_graph );
        ch_graph[v].order = road_graph[v].db_id();
        ch_graph[v].id = road_graph[v].db_id();
        node_map[ch_graph[v].order] = v;
    }
    for ( auto ei = edges( road_graph ).first; ei != edges( road_graph).second; ei++ ) {
        CHVertex s = source(*ei, road_graph);
        CHVertex t = target(*ei, road_graph);
        auto ne = add_edge( s, t, ch_graph );
        BOOST_ASSERT(ne.second);
        // only pedestrian for now
        ch_graph[ne.first].weight = road_graph[*ei].length();// * 6000 / 60.0;
    }

    std::cout << num_vertices( ch_graph ) << " " << num_edges( ch_graph ) << std::endl;
    std::cout << "contracting ..." << std::endl;

    contract_graph( ch_graph, node_map, spriv->middle_node );
#else
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
                                                  "select o1.id as id1, o2.id as id2, weight, o3.id as mid, 0 as dir\n"
                                                  "from ch.query_graph\n"
                                                  "left join ch.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  ", ch.ordered_nodes as o1, ch.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 1 > 0\n"

                                                  // union with the downward part
                                                  "union all\n"
                                                  "select o1.id as id1, o2.id as id2, weight, o3.id as mid, 1 as dir\n"
                                                  "from ch.query_graph\n"
                                                  "left join ch.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  ", ch.ordered_nodes as o1, ch.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 2 > 0\n"
                                                  ") t order by id1, dir"
                                                  );
        Db::ResultIterator it_end;

        std::vector<std::pair<uint32_t,uint32_t>> targets;
        std::vector<CHEdgeProperty> properties;
        std::vector<uint16_t> up_degrees;

        uint32_t node = 0;
        uint16_t upd = 0;
        for ( ; res_it != it_end; res_it++ ) {
            Db::RowValue res_i = *res_it;
            uint32_t id1 = res_i[0].as<uint32_t>() - 1;
            uint32_t id2 = res_i[1].as<uint32_t>() - 1;
            int dir = res_i[4];
            if ( id1 > node ) {
                //std::cout << "upd " << upd << std::endl;
                up_degrees.push_back( upd );
                node = id1;
                upd = 0;
            }
            //std::cout << id1 << "->" << id2;
            if ( dir == 0 ) {
                //std::cout << "+";
                upd++;
            }
            CHEdgeProperty p;
            p.cost = res_i[2];
            p.is_shortcut = 0;
            if ( !res_i[3].is_null() ) {
                uint32_t middle = res_i[3].as<uint32_t>() - 1;
                // we have a middle node, it is a shortcut
                p.is_shortcut = 1;
                //std::cout << " S";
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
    
#endif
}

void CHPlugin::pre_process( Request& request )
{
    REQUIRE( vertex_exists( request.origin(), graph_.road() ) );
    REQUIRE( vertex_exists( request.destination(), graph_.road() ) );

    request_ = request;

    result_.clear();
}

std::pair<std::list<Road::Vertex>, float> dijkstra_query( const Road::Graph& graph, Road::Vertex origin, Road::Vertex destination )
{
    std::pair<std::list<Road::Vertex>, float> ret;

    const float infinity = std::numeric_limits<float>::max();

    typedef std::vector<float> PotentialMap;
    PotentialMap potential( num_vertices(graph), infinity );

    std::vector<Road::Vertex> pred_map( num_vertices( graph ) );

    struct DijkstraAbort {};

    struct DijkstraAborter : public boost::base_visitor<DijkstraAborter>
    {
        typedef boost::on_examine_vertex event_filter;
        DijkstraAborter( const Road::Vertex& destination ) : destination_(destination) {}

        void operator()( const Road::Vertex& v, const Road::Graph& )
        {
            if ( v == destination_ ) {
                throw DijkstraAbort();
            }
        }
        Road::Vertex destination_;
    };

    auto weight_map_fn = [&graph]( const Road::Edge& e ) {
        if ( (graph[e].traffic_rules() & TrafficRulePedestrian) == 0 ) {
            return std::numeric_limits<float>::max();
        }
        return roundf(graph[e].length() * 100.0);
    };
    auto weight_map = boost::make_function_property_map<Road::Edge, float, decltype(weight_map_fn)>( weight_map_fn );

    DijkstraAborter vis( destination );
    bool path_found = false;
    try {
        boost::dijkstra_shortest_paths_no_color_map( graph,
                                                     origin,
                                                     &pred_map[0],
                                                     &potential[0],
                                                     weight_map,
                                                     boost::get( boost::vertex_index, graph ),
                                                     std::less<float>(),
                                                     boost::closed_plus<float>(),
                                                     infinity,
                                                     0.0,
                                                     boost::make_dijkstra_visitor(vis)
                                                     );
    }
    catch ( DijkstraAbort& )
    {
        path_found = true;
    }

    auto& path = ret.first;
    if ( path_found ) {
        Road::Vertex current = destination;

        while ( current != origin ) {
            path.push_front( current );
            current = pred_map[ current ];
        }
        path.push_front( origin );
    }

    // path cost
    ret.second = potential[destination];
    return ret;
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
            //std::cout << "end" << std::endl;
            break;
        }

        // interleave directions
        dir = 1 - dir;

        if ( vertex_queue[dir].empty() )
            dir = 1 - dir;

        Vertex min_v = vertex_queue[dir].top();
        vertex_queue[dir].pop();
        CostType min_pi = get( potential_map[dir], min_v );
        //std::cout << dir << " " << min_v << "(" << node_id[min_v] << ") " << min_pi << std::endl;
        
        {
            CostType min_pi2 = get( potential_map[1-dir], min_v );
            // if min_pi2 is not infinity, it means this node has already been seen
            // in the other direction
            // so it is a candidate top node
            if ( min_pi2 < infinity ) {
                //std::cout << "*** candidate top node " << node_id[min_v] << std::endl;
            }
            if ( min_pi + min_pi2 < total_cost ) {
                top_node = min_v;
                //std::cout << "*** top node " << node_id[top_node] << std::endl;
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
                //std::cout << "upward " << vv << "(" << node_id[vv] << ") " << cost << " " << new_pi << std::endl;
                if ( min_pi + cost < new_pi ) {
                    // relax edge
                    put( potential_map[dir], vv, min_pi + cost );
                    vertex_queue[dir].push( vv );
                    //std::cout << "up pred of " << node_id[vv] << " = " << node_id[min_v] << std::endl;
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
                //std::cout << "downward " << node_id[vv] << " " << cost << " " << new_pi << std::endl;
                if ( min_pi + cost < new_pi ) {
                    // relax edge
                    put( potential_map[dir], vv, min_pi + cost );
                    vertex_queue[dir].push( vv );
                    //std::cout << "dn pred of " << node_id[vv] << " = " << node_id[min_v] << std::endl;
                    predecessor[dir][vv] = min_v;
                }
            }
        }
    }

    if ( !path_found ) {
        std::cout << "NO PATH FOUND" << std::endl;
        return returned_path;
    }
    std::cout << "PATH FOUND" << std::endl;

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
        return float(e.property().cost);
    };
    auto weight_map = boost::make_function_property_map<CHQuery::edge_descriptor, float, decltype(weight_map_fn)>( weight_map_fn );
    float ret_cost = std::numeric_limits<float>::max();
    auto path = bidirectional_ch_dijkstra( *spriv->ch_query, ch_origin, ch_destination, weight_map, ret_cost, spriv->node_id );

    auto& ret_path = ret.first;
    unpack_path( path.begin(), path.end(), spriv->middle_node, std::back_inserter( ret_path ) );
    ret.second = ret_cost;

    return ret;
}

void CHPlugin::process()
{
    const Road::Graph& graph = graph_.road();

    SPriv* spriv = (SPriv*)spriv_;
#if 1
    while (true) {
        CHVertex ch_origin = rand() % spriv->node_id.size();
        CHVertex ch_destination = rand() % spriv->node_id.size();
#else
    {
        CHVertex ch_origin, ch_destination;
        for ( size_t i = 0; i < spriv->node_id.size(); i++ ) {
            if ( spriv->node_id[i] == 26773 ) {
                ch_origin = i;
            }
            else if ( spriv->node_id[i] == 4401 ) {
                ch_destination = i;
            }
        }
#endif

        db_id_t origin_id = spriv->node_id[ch_origin];
        db_id_t destination_id = spriv->node_id[ch_destination];
        Road::Vertex origin;
        Road::Vertex destination;
        for ( auto it = vertices(graph).first; it != vertices(graph).second; it++ ) {
            if ( graph[*it].db_id() == origin_id ) {
                origin = *it;
            }
            else if ( graph[*it].db_id() == destination_id ) {
                destination = *it;
            }
        }

        std::cout << "From " << origin_id << " to " << destination_id << std::endl;

        auto ch_ret = ch_query( spriv, ch_origin, ch_destination );
        auto dijkstra_ret = dijkstra_query( graph, origin, destination );

#if 0
        std::cout << "Path" << std::endl;
        auto ch_it = ch_ret.first.begin();
        auto dj_it = dijkstra_ret.first.begin();
        for ( ; (ch_it != ch_ret.first.end()) && (dj_it != dijkstra_ret.first.end()); ch_it++, dj_it++ ) {
            db_id_t ch_id = spriv->node_id[*ch_it];
            db_id_t dj_id = graph[*dj_it].db_id();
            if ( ch_id != dj_id ) {
                std::cout << "Different paths ! CH: " << ch_id << " Dijkstra: " << dj_id << std::endl;
                //break;
            }
            std::cout << ch_id << std::endl;
        }
#endif

        float ch_cost = ch_ret.second;
        float dijkstra_cost = dijkstra_ret.second;
        std::cout << ch_cost << "(CH) - " << dijkstra_cost << "(Dijkstra)" << std::endl;
        BOOST_ASSERT( fabs((ch_cost - dijkstra_cost) / ch_cost) < 0.01 );
    }
}

} // namespace Tempus

DECLARE_TEMPUS_PLUGIN( "ch_plugin", Tempus::CHPlugin )

