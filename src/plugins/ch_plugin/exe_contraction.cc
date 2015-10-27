#include <string>

#include <chrono>
#include <unordered_set>

#include <boost/heap/d_ary_heap.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/property_map/function_property_map.hpp>

#include "application.hh"

template<class It>
boost::iterator_range<It> pair_range(std::pair<It, It> const& p)
{
    return boost::make_iterator_range(p.first, p.second);
}

struct VertexProperty
{
    Tempus::db_id_t id;
    uint32_t order;
};

struct EdgeProperty
{
    int weight;
};


typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> CHGraph;
typedef typename boost::graph_traits<CHGraph>::vertex_descriptor CHVertex;
typedef typename boost::graph_traits<CHGraph>::edge_descriptor CHEdge;

inline std::chrono::milliseconds mstime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

using TCost = int;
using TTargetSet = std::set<CHVertex>;

using namespace std;

namespace
{
struct TEdge
{
    CHVertex from;
    CHVertex to;
    TCost cost;
};


struct Shortcut
{
    CHVertex from;
    CHVertex to;
    TCost cost;
    CHVertex contracted;
};

}

// If set to 1, the graph is really contracted (i.e. a node and its references are removed from the graph after contraction)
// If set to 0, edges are filtered during the contraction (retaining nodes of upper order)
// Actually reducing the graph seems a bit faster
#define REDUCE_GRAPH 1

map<CHVertex, TCost> witness_search(const CHGraph& graph, CHVertex contracted_node, CHVertex source, const TTargetSet& targets, TCost cutoff, int& search_space)
{
    struct DijkstraNode
    {
        CHVertex node;
        TCost cost;
        int hops;
        int order;
    };

    class CompareNode
    {
    public:
        bool operator()(const DijkstraNode& node1, const DijkstraNode& node2) const
        {
            // node2 has higher priority if it has a smaller cost
            // (DijkstraNode::order used in case of cost equality, for non-random heap.pop())
            return node1.cost > node2.cost || (node1.cost == node2.cost && node1.order > node2.order);
        }
    };

    // Dijkstra from one source to multiple targets.
    REQUIRE(!targets.empty());

    TTargetSet found; // targets found
    map<CHVertex, TCost> relaxed; // minimal costs of relaxed nodes (from 'source')
    relaxed[source] = 0; // the cost from 'source' to 'source' is 0
    search_space = 1; // search-space of Dijkstra (used for node-ordering)

    int order = 0;
#if 1
    priority_queue<DijkstraNode, vector<DijkstraNode>, CompareNode> heap; // priority queue
#else
    CompareNode cmp;
    boost::heap::d_ary_heap< DijkstraNode, boost::heap::arity<2>, boost::heap::compare< CompareNode >, boost::heap::mutable_<true> > heap( cmp );
#endif
    heap.push( {source, 0, 1, order++} );

    while( !heap.empty() )
    {
        // Get less costly node in priority queue:
        const DijkstraNode& top = heap.top();
        CHVertex node = top.node;
        TCost cost = top.cost;
        int num_hops = top.hops;
        heap.pop();

        if( targets.find( node ) != targets.end() ) // found one of the targets
        {
            found.emplace( node );

            if( found == targets ) // all targets have been found
            {
                break;
            }
        }

        for ( auto succ : pair_range( out_edges( node, graph ) ) )
        {
            CHVertex successor_node = target( succ, graph );
#if REDUCE_GRAPH
            if( successor_node == contracted_node ) // ignore contracted node
                continue;
#else
            if( successor_node <= contracted_node ) // ignore contracted node and lower nodes
                continue;
#endif

            TCost vu_cost = cost + graph[succ].weight;
            if(cutoff && vu_cost > cutoff) // do not search beyond 'cutoff'
            {
                continue;
            }

            auto iter = relaxed.find( successor_node );
            if( iter == relaxed.end() || vu_cost < iter->second )
            {
                // Add node to priority queue:
                heap.push( {successor_node, vu_cost, num_hops + 1, order++} );
                relaxed[successor_node] = vu_cost;

                if(num_hops + 1 > search_space)
                    search_space = num_hops + 1;
            }
        }
    }

    map<CHVertex, TCost> target_costs;
    for(CHVertex t : found)
        target_costs.emplace( t, relaxed[t] );
    return target_costs;
}


std::vector<TEdge> get_contraction_shortcuts( const CHGraph& graph, CHVertex v )
{
    // Contraction of 'node'.

    vector<TEdge> shortcuts;
    for ( auto uv : pair_range( in_edges( v, graph ) ) )
        //    if(!successors.empty() && !predecessors.empty() && !(successors.size() == 1 && predecessors.size() == 1 && successors.back().node == predecessors.back().node))
    {
        TTargetSet targets;
        TCost mx = 0;
        CHVertex u = source( uv, graph );

#if !REDUCE_GRAPH
        if ( u < v ) // only consider upper predecessors
            continue;
#endif

        //std::cout << s << " " << node << std::endl;
        for ( auto vw : pair_range( out_edges( v, graph ) ) )
        {
            CHVertex w = target( vw, graph );
            if ( w == u )
                continue;
#if !REDUCE_GRAPH
            if ( w < v ) // only consider upper successors
                continue;
#endif

            targets.emplace( w );

            if ( graph[vw].weight > mx )
                mx = graph[vw].weight;
        }

        if ( targets.empty() )
            continue;

        // It is not useful to search shortcuts beyond 'cutoff'
        TCost cutoff = graph[uv].weight + mx;

        // Perform Dijkstra from 'predecessor' to all 'targets' while ignoring 'node'
        int _ = 0; // not used
        auto target_costs = witness_search( graph, v, u, targets, cutoff, _ );

        for ( auto vw : pair_range( out_edges( v, graph ) ) )
        {
            CHVertex w = target( vw, graph );
            if ( w == u )
                continue;
#if !REDUCE_GRAPH
            if ( w < v ) // only consider upper successors
                continue;
#endif

            auto iter_shortcut = target_costs.find( w );
            TCost uvw_cost = graph[uv].weight + graph[vw].weight;
            if( iter_shortcut == target_costs.end() || iter_shortcut->second > uvw_cost )
            {
                // If no shortest path was found from 'predecessor' to 'successor' during the Dijkstra
                // propagation, then it means the shortest path from 'predecessor' to 'successor' is
                // <predecessor, node, successor>, and a shortcut must be added.
                shortcuts.push_back( {u, w, uvw_cost} );
            }
        }
    }

    // cout << "Contracted " << node << "." << endl;
    return shortcuts;
}

void add_edge( CHGraph& graph, CHVertex u, CHVertex v, TCost cost )
{
    bool found = false;
    CHEdge e;
    boost::tie( e, found ) = edge( u, v, graph );
    if ( !found ) {
        boost::tie( e, found ) = add_edge( u, v, graph );
    }
    // update the cost
    graph[e].weight = cost;
}

vector<Shortcut> contract_graph( CHGraph& graph )
{
    cout << "contract_graph" << endl;

    vector<Shortcut> r;
    chrono::milliseconds t = mstime();

    for ( auto node : pair_range( vertices( graph ) ) )
    {
        if(node % 100000 == 0)
            cout << "Contracting nodes " << node << "... [elapsed=" << (mstime() - t).count() << "ms]" << endl;

        // Single node contraction:
        vector<TEdge> shortcuts = get_contraction_shortcuts( graph, node );

        // Update graph after contraction:
        for(const TEdge& edge : shortcuts)
        {
            r.push_back( {edge.from, edge.to, edge.cost, node} );

            // add the shortcut to the graph
            add_edge( graph, edge.from, edge.to, edge.cost );
        }

#if REDUCE_GRAPH
        // clear any reference to this node
        clear_vertex( node, graph );
#endif
    }
    // IDF: 7289ms

    cout << "Contracted entire graph in " << (mstime() - t).count() << "ms." << endl;
    return r;
}

#if 0
template <typename Graph>
using MiddleNodeMap = std::map<std::pair<typename Graph::vertex_descriptor, typename Graph::vertex_descriptor>, typename Graph::vertex_descriptor>;

///
/// Search witness paths for u-v-w from the vertex u to a set of target vertices
///
/// @param graph The input graph
/// @param weight_map A readable property map that gives the weight of an edge
/// @param u the predecessor vertex
/// @param v the current vertex to contract
/// @param targets list of successor vertices
/// @param max_cost the maximum cost to consider. If a path longer than that is found, shortcut
/// @returns a vector of shortest path costs for each u-v-target[i]
template <
    typename Graph,
    typename Weight,
    typename WeightMap,
    typename Vertex = typename boost::graph_traits<Graph>::vertex_descriptor,
    typename Edge = typename boost::graph_traits<Graph>::edge_descriptor
    >
std::vector<Weight> witness_search( const Graph& graph, const WeightMap& weight_map, Vertex u, Vertex v, const std::vector<Edge>& targets, Weight max_cost = Weight() )
{
    const Weight infinity = std::numeric_limits<Weight>::max();

    // shortests paths to target nodes
    std::vector<Weight> costs;
    costs.resize( targets.size() );

    std::list<std::pair<Vertex, size_t>> remaining_targets;
    size_t idx = 0;
    for ( Edge e : targets ) {
        Vertex w = target( e, graph );
        if ( w == u ) {
            // avoid cycles
            idx++;
            continue;
        }
        remaining_targets.push_back( std::make_pair(w, idx) );
        costs[idx] = infinity;
        idx++;
    }
    
    //
    // one-to-many dijkstra search
    // on the upward graph
    // and excluding the middle node (v)
    typedef std::vector<Weight> PotentialMap;
    PotentialMap potential( num_vertices( graph ), infinity );
    typedef Weight* PotentialPMap;
    PotentialPMap potential_map = &potential[0];

    auto cmp = boost::make_indirect_cmp( std::greater<Weight>(), potential_map );

    typedef boost::heap::d_ary_heap< Vertex, boost::heap::arity<4>, boost::heap::compare< decltype(cmp) >, boost::heap::mutable_<true> > VertexQueue;
    VertexQueue vertex_queue( cmp );

    //std::cout << "witness search " << graph[u].id << " - " << graph[v].id << std::endl;

    vertex_queue.push( u ); 
    put( potential_map, u, 0.0 );

    while ( !vertex_queue.empty() ) {
        Vertex min_v = vertex_queue.top();
        vertex_queue.pop();
        Weight min_pi = get( potential_map, min_v );
        //std::cout << "min_v " << graph[min_v].id << std::endl;

        // did we reach a target node ?
        // FIXME optimize
        auto it = std::find_if( remaining_targets.begin(), remaining_targets.end(),
                                [&min_v](const std::pair<Vertex,size_t>& p)
                                { return p.first == min_v; }
                                );
        if ( it != remaining_targets.end() ) {
            // store the cost to get this target node
            //std::cout << "witness found on " << graph[u].id << " - " << graph[v].id << " - " << graph[min_v].id << " = " << min_pi << std::endl;
            costs[it->second] = min_pi;
            remaining_targets.erase( it );
        }
        if ( remaining_targets.empty() ) {
            // no target node to process anymore, returning
            return costs;
        }

        for ( auto oei = out_edges( min_v, graph ).first;
              oei != out_edges( min_v, graph ).second;
              oei++ ) {
            Vertex vv = target( *oei, graph );
            //std::cout << graph[vv].id << std::endl;
            // exclude the middle node
            if ( vv == v )
                continue;
            double new_pi = get( potential_map, vv );
            double cost = get( weight_map, *oei );
            if ( min_pi + cost > max_cost ) {
                continue;
            }

            //std::cout << "vv = " << vv << " order = " << graph[vv].order << " min_pi " << min_pi << " cost " << cost << " new_pi " << new_pi << std::endl;
            if ( min_pi + cost < new_pi ) {
                // relax edge
                put( potential_map, vv, min_pi + cost );
                vertex_queue.push( vv );
            }
        }
    }

    //std::cout << "no path" << std::endl;

    return costs;
}

///
/// Main contraction function
/// @param graph The graph to contract. Edges will be added to represent shortcuts.
/// @param node_it An iterator to the beginning of a sequence of nodes, sorted by CH order
/// @param node_end An iterator to the end of sequence of nodes, sorted by CH order
/// @param order_map A readable property map that gives the order of a node
/// @param weight_map A readable-writable property map where edge weights are stored (and inserted / updated)
/// @param middle_node The output value: a structure where shortcuts are 
template <typename Graph,
          typename NodeIterator,
          typename OrderMap,
          typename WeightMap>
void contract_graph( Graph& graph, NodeIterator node_it, NodeIterator node_end, const OrderMap& order_map, WeightMap& weight_map, MiddleNodeMap<Graph>& middle_node )
{
    using Vertex = typename Graph::vertex_descriptor;
    using Edge = typename Graph::edge_descriptor;
    using Weight = typename boost::property_traits<WeightMap>::value_type;
    static_assert( std::is_same<typename std::iterator_traits<NodeIterator>::value_type, typename Graph::vertex_descriptor>::value, "Wrong NodeIterator value_type" );
    // FIXME check order_map type
    // FIXME check weight_map type

    size_t v_idx = 0;
    for ( ; node_it != node_end; node_it++, v_idx++ ) {
        Vertex v = *node_it;

        // witness search

        if ( v_idx % 100 == 0 ) {
            std::cout << v_idx << std::endl;
        }

        std::vector<Edge> successors;
        successors.reserve( out_degree( v, graph ) );

        Weight max_cost = Weight();
        for ( auto oei = out_edges( v, graph ).first; oei != out_edges( v, graph ).second; oei++ ) {
            auto succ = target( *oei, graph );
            // only retain successors of higher order
            if ( get( order_map, succ ) < get( order_map, v ) )
                continue;
            //std::cout << "succ " << road_graph[succ].db_id() << std::endl;

            successors.push_back( *oei );

            // get the max cost that will be used for the witness search
            Weight cost = get( weight_map, *oei );
            if ( cost > max_cost )
                max_cost = cost;
        }

        //std::cout << "#successors " << successors.size() << std::endl;
        if ( successors.empty() )
            continue;

        struct Shortcut
        {
            Vertex u;
            Vertex w;
            Weight cost;
            Shortcut( Vertex au, Vertex aw, Weight acost ) :u(au), w(aw), cost(acost) {}
        };
        std::vector<Shortcut> shortcuts;

        for ( auto iei = in_edges( v, graph ).first; iei != in_edges( v, graph ).second; iei++ ) {
            auto pred = source( *iei, graph );
            // only retain predecessors of higher order
            if ( get( order_map, pred ) < get( order_map, v ) )
                continue;

            Weight pred_cost = get( weight_map, *iei );
            //std::cout << "u " << ch_graph[pred].id << " v " << ch_graph[v].id << " uv cost " << pred_cost << std::endl;

            //std::cout << "witness search between " << pred << " and " << v << " pred cost " << pred_cost << std::endl;
            std::vector<Weight> witness_costs = witness_search( graph, weight_map, pred, v, successors, pred_cost + max_cost );

            size_t i = 0;
            for ( auto vw_it = successors.begin(); vw_it != successors.end(); vw_it++, i++ ) {
                Vertex w = target( *vw_it, graph );
                if ( w == pred ) {
                    continue;
                }
                Weight uvw_cost = pred_cost + get( weight_map, *vw_it );
                if ( witness_costs[i] > uvw_cost ) {
                    // need to add a shortcut between pred and succ

                    // need to store them in a temp array, because adding edges during the loop
                    // would result in strange behaviour
                    shortcuts.emplace_back( pred, w, uvw_cost );
                    // update the middle node
                    middle_node[std::make_pair(pred, w)] = v;
                }
            }
        }

        for ( const auto& s : shortcuts ) {
            bool found;
            Edge e;
            // update the cost or add a new shortcut
            std::tie(e, found) = edge( s.u, s.w, graph );
            if ( found ) {
                BOOST_ASSERT( get( weight_map, e ) >= s.cost );
                put( weight_map, e, s.cost );
            }
            else {
                std::tie(e, found) = add_edge( s.u, s.w, graph );
                BOOST_ASSERT( found );
                put( weight_map, e, s.cost );
            }
        }

        // remove the current node ??
        remove_vertex( v, graph );
    }
}
#endif

int main( int argc, char *argv[] )
{
    using namespace Tempus;

    const std::string& dbstring = argc > 1 ? argv[1] : "dbname=tempus_test_db";

    Application* app = Application::instance();

    app->connect( dbstring );
    app->pre_build_graph();
    std::cout << "building the graph...\n";

    app->build_graph( /*consistency_check*/ false, "tempus" );

    const Multimodal::Graph& graph = *app->graph();
    const Road::Graph& road_graph = graph.road();

#if 1
    CHGraph ch_graph;

    // get nodes, ordered
    std::vector<db_id_t> nodes; // order -> id
    std::map<db_id_t, uint32_t> node_id_order; // id -> order
    Db::Connection conn( dbstring );
    {
        Db::ResultIterator res_it = conn.exec_it( "select node_id from ch.ordered_nodes order by sort_order asc" );
        Db::ResultIterator it_end;

        uint32_t i = 0;
        for ( ; res_it != it_end; res_it++, i++ ) {
            Db::RowValue res_i = *res_it;
            db_id_t db_id = res_i[0];
            nodes.push_back( db_id );
            //std::cout << i << " -> " << db_id << std::endl;
            node_id_order[db_id] = i;
        }
    }

    std::map<db_id_t, Road::Vertex> id_map; // id -> vertex
    for ( auto it = vertices( road_graph ).first; it != vertices( road_graph ).second; it++ ) {
        CHVertex v = add_vertex( ch_graph );
        id_map[road_graph[*it].db_id()] = *it;
        ch_graph[v].id = road_graph[*it].db_id();
    }

    conn.exec( "CREATE SCHEMA IF NOT EXISTS ch2" );
    conn.exec( "DROP TABLE IF EXISTS ch2.query_graph CASCADE" );
    conn.exec( "CREATE TABLE ch2.query_graph (id SERIAL PRIMARY KEY, node_inf BIGINT NOT NULL, node_sup BIGINT NOT NULL, contracted_id BIGINT, weight INT NOT NULL, constraints INT NOT NULL)" );

    for ( uint32_t order = 0; order < nodes.size(); order++ ) {
        Road::Vertex v = id_map[nodes[order]];
        for ( auto it = out_edges( v, road_graph ).first; it != out_edges( v, road_graph ).second; it++ ) {
            if ( (road_graph[*it].traffic_rules() & TrafficRulePedestrian) == 0 ) {
                continue;
            }
            Road::Vertex s = target( *it, road_graph );
            db_id_t id = road_graph[s].db_id();
            //tgraph.addEdge( order, node_id_order[id], int(road_graph[*it].length()*100.0) );
            bool added = false;
            CHEdge e;
            boost::tie( e, added ) = add_edge( order, node_id_order[id], ch_graph );
            BOOST_ASSERT( added );
            ch_graph[e].weight = std::max(int(road_graph[*it].length()*100.0),1);
        }
    }

    std::vector<Shortcut> shortcuts = contract_graph( ch_graph );
    std::cout << "# shortcuts" << shortcuts.size() << std::endl;

    // copy original edges
    conn.exec( "INSERT INTO ch2.query_graph (node_inf, node_sup, weight, constraints) "
               "SELECT node_from, node_to, greatest((length*100)::int,1), 1025 FROM tempus.road_section" );

    // write shortcuts
    conn.exec( "BEGIN" );
    for ( const Shortcut& s : shortcuts )
    {
        std::ostringstream ss;
        ss << "(" << nodes[s.from] << "," << nodes[s.to] << "," << nodes[s.contracted] << "," << s.cost << ",1025)";
        conn.exec( "INSERT INTO ch2.query_graph (node_inf, node_sup, contracted_id, weight, constraints) VALUES " + ss.str() );
    }
    conn.exec( "COMMIT" );
#else
    CHGraph ch_graph;
    std::map<db_id_t, uint32_t> id_map;
    // copy the (read-only) road_graph to ch_graph
    for ( auto it = vertices( road_graph ).first; it != vertices( road_graph ).second; it++ ) {
        add_vertex( ch_graph );
        id_map[road_graph[*it].db_id()] = *it;
        ch_graph[*it].id = road_graph[*it].db_id();
    }
    for ( auto it = edges( road_graph ).first; it != edges( road_graph ).second; it++ ) {
        bool added;
        CHEdge e;
        boost::tie( e, added ) = add_edge( source( *it, road_graph ), target( *it, road_graph ), ch_graph );
        BOOST_ASSERT( added );
        ch_graph[e].weight = road_graph[*it].length();
    }

    MiddleNodeMap<CHGraph> middle_node;

    // nodes in ascending order
    std::vector<CHVertex> nodes;
    Db::Connection conn( dbstring );
    {
        Db::ResultIterator res_it = conn.exec_it( "select node_id from ch.ordered_nodes order by sort_order asc" );
        Db::ResultIterator it_end;

        uint32_t i = 0;
        for ( ; res_it != it_end; res_it++, i++ ) {
            Db::RowValue res_i = *res_it;
            db_id_t db_id = res_i[0];
            nodes.push_back( id_map[db_id] );
            ch_graph[ id_map[db_id] ].order = i;
        }
    }

    auto order_map_fn = [&ch_graph] ( const CHVertex& v ) {
        return ch_graph[v].order;
    };
    auto order_map = boost::function_property_map< decltype(order_map_fn), CHVertex >( order_map_fn );

    auto weight_map_fn = [&ch_graph] ( const CHEdge& e ) -> float& {
        return ch_graph[e].weight;
    };
    auto weight_map = boost::function_property_map< decltype(weight_map_fn), CHEdge >( weight_map_fn );

    contract_graph( ch_graph, nodes.begin(), nodes.end(), order_map, weight_map, middle_node );

    for ( auto vit = vertices( ch_graph ).first; vit != vertices( ch_graph ).second; vit++ ) {
        CHVertex u = *vit;
        for ( auto eit = out_edges( u, ch_graph ).first; eit != out_edges( u, ch_graph ).second; eit++ ) {
            CHVertex v = target( *eit, ch_graph );
            if ( ch_graph[v].order < ch_graph[u].order )
                continue;

            std::cout << ch_graph[u].id << ";" << ch_graph[v].id << ";";
            auto fit = middle_node.find( std::make_pair(u,v) );
            if ( fit != middle_node.end() ) {
                std::cout << ch_graph[fit->second].id;
            }
            std::cout << ";" << ch_graph[*eit].weight << std::endl;
        }
    }
#endif
}
