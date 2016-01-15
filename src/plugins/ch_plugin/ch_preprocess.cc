/**
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

#include "common.hh"
#include "ch_preprocess.hh"
#include "utils/timer.hh"

#include <chrono>
#include <unordered_set>
#include <queue>

using namespace Tempus;
using namespace std;

using TTargetSet = std::set<CHVertex>;

using TNodeContractionCost = int;
using TRemainingNodeSet = std::unordered_set<CHVertex>;

struct TEdge
{
    CHVertex from;
    CHVertex to;
    TCost cost;
};

void add_edge_or_update( CHGraph& graph, CHVertex u, CHVertex v, TCost cost )
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

void get_node_edge_impact( const CHGraph& graph, CHVertex v, int& nb_added_edges, int& max_search_space )
{
    // Contraction of 'node'.

    nb_added_edges = 0;
    max_search_space = 0;

    for ( auto uv : pair_range( in_edges( v, graph ) ) )
    {
        TTargetSet targets;
        TCost mx = 0;
        CHVertex u = source( uv, graph );

        //std::cout << s << " " << node << std::endl;
        for ( auto vw : pair_range( out_edges( v, graph ) ) )
        {
            CHVertex w = target( vw, graph );
            if ( w == u )
                continue;

            targets.emplace( w );

            if ( graph[vw].weight > mx )
                mx = graph[vw].weight;
        }

        if ( targets.empty() )
            continue;

        // It is not useful to search shortcuts beyond 'cutoff'
        TCost cutoff = graph[uv].weight + mx;

        // Perform Dijkstra from 'predecessor' to all 'targets' while ignoring 'node'
        int search_space = 0;
        auto target_costs = witness_search( graph, v, u, targets, cutoff, search_space );

        if ( search_space > max_search_space )
            max_search_space = search_space;

        for ( auto vw : pair_range( out_edges( v, graph ) ) )
        {
            CHVertex w = target( vw, graph );
            if ( w == u )
                continue;

            auto iter_shortcut = target_costs.find( w );
            TCost uvw_cost = graph[uv].weight + graph[vw].weight;
            if( iter_shortcut == target_costs.end() || iter_shortcut->second > uvw_cost )
            {
                // If no shortest path was found from 'predecessor' to 'successor' during the Dijkstra
                // propagation, then it means the shortest path from 'predecessor' to 'successor' is
                // <predecessor, node, successor>, and a shortcut must be added.
                nb_added_edges++;
            }
        }
    }
}

TNodeContractionCost get_node_cost( const CHGraph& graph,
                                    CHVertex node,
                                    const vector<int>& hierarchyDepths,
                                    db_id_t nodeId64)
{
    int nbRemovedEdges = 0;
    int nbAddedEdges = 0;
    int maxSearchSpace = 0;

    nbRemovedEdges = in_degree( node, graph ) + out_degree( node, graph );

    get_node_edge_impact(graph, node, nbAddedEdges, maxSearchSpace);

    // The "edge difference" is the number of shortcuts created
    // when contracting 'node' minus the number of edges removed.
    int edgeDiff = nbAddedEdges - nbRemovedEdges;
    REQUIRE(abs(edgeDiff) <= 2000); // 32-bit limit

    int depth = hierarchyDepths[node];

    // using 'nodeId64' to get a more "deterministic" ordering (with the same order as in Python)
    return edgeDiff * 1000000 + depth * 100000 + maxSearchSpace * 10000 + (nodeId64 % 10000);
}


inline bool is_node_independent(const CHGraph& graph,
                                CHVertex node,
                                const vector<TNodeContractionCost>& nodeCosts,
                                TNodeContractionCost nodeCost)
{
    // A node is 'independent' if all its neighbors, and the neighbors
    // of its neighbors have a higher cost.

    for ( const auto& pred : pair_range( in_edges( node, graph ) ) )
    {
        CHVertex u = source( pred, graph );
        if ( nodeCosts[u] < nodeCost )
            return false;

        for ( const auto& pred2 : pair_range( in_edges( u, graph ) ) )
        {
            CHVertex v = source( pred2, graph );
            if ( nodeCosts[v] < nodeCost )
                return false;
        }
        for ( const auto& succ2 : pair_range( out_edges( u, graph ) ) )
        {
            CHVertex v = target( succ2, graph );
            if ( nodeCosts[v] < nodeCost )
                return false;
        }
    }

    for ( const auto& succ : pair_range( out_edges( node, graph ) ) )
    {
        CHVertex u = target( succ, graph );
        if ( nodeCosts[u] < nodeCost )
            return false;

        for ( const auto& pred2 : pair_range( in_edges( u, graph ) ) )
        {
            CHVertex v = source( pred2, graph );
            if ( nodeCosts[v] < nodeCost )
                return false;
        }
        for ( const auto& succ2 : pair_range( out_edges( u, graph ) ) )
        {
            CHVertex v = target( succ2, graph );
            if ( nodeCosts[v] < nodeCost )
                return false;
        }
    }

    return true;
}


template <typename Foo>
void apply_on_1_neighbourhood( const CHGraph& graph, CHVertex node, Foo foo )
{
    for ( const auto& pred : pair_range( in_edges( node, graph ) ) )
    {
        CHVertex u = source( pred, graph );
        foo( u );
    }

    for ( const auto& succ : pair_range( out_edges( node, graph ) ) )
    {
        CHVertex u = target( succ, graph );
        foo( u );
    }
}

template <typename Foo>
void apply_on_2_neighbourhood( const CHGraph& graph, CHVertex node, Foo foo )
{
    for ( const auto& pred : pair_range( in_edges( node, graph ) ) )
    {
        CHVertex u = source( pred, graph );
        foo( u );

        for ( const auto& pred2 : pair_range( in_edges( u, graph ) ) )
        {
            CHVertex v = source( pred2, graph );
            foo( v );
        }
        for ( const auto& succ2 : pair_range( out_edges( u, graph ) ) )
        {
            CHVertex v = target( succ2, graph );
            foo( v );
        }
    }

    for ( const auto& succ : pair_range( out_edges( node, graph ) ) )
    {
        CHVertex u = target( succ, graph );
        foo( u );

        for ( const auto& pred2 : pair_range( in_edges( u, graph ) ) )
        {
            CHVertex v = source( pred2, graph );
            foo( v );
        }
        for ( const auto& succ2 : pair_range( out_edges( u, graph ) ) )
        {
            CHVertex v = target( succ2, graph );
            foo( v );
        }
    }
}

vector<CHVertex> get_independent_node_set( const CHGraph& graph,
                                           vector<CHVertex>::const_iterator nodeBeginIt,
                                           int numNodes,
                                           const vector<TNodeContractionCost>& nodeCosts )
{
    // Get all independent nodes from the sequence [nodeBeginIt : NodeBeginIt+numNodes]

    REQUIRE(!nodeCosts.empty());
    REQUIRE(numNodes > 0);

    vector<CHVertex> returnValue;
    unordered_set<CHVertex> excluded;
    auto nodeEndIt = nodeBeginIt + numNodes;
    for (auto nodeIt=nodeBeginIt; nodeIt!=nodeEndIt; ++nodeIt)
    {
        CHVertex nodeID = *nodeIt;
        if (excluded.find(nodeID) != excluded.end())
            continue;
        REQUIRE(nodeID < nodeCosts.size());
        TNodeContractionCost nodeCost = nodeCosts[nodeID];

        if ( !is_node_independent( graph, nodeID, nodeCosts, nodeCost ) )
            continue;

        apply_on_2_neighbourhood( graph, nodeID, [&excluded]( CHVertex u ) {
                excluded.emplace( u );
            });

        returnValue.push_back(nodeID);
    }

    return returnValue;
}


namespace Tempus
{

vector<CHVertex> order_graph( CHGraph& graph, std::function<db_id_t(CHVertex)> node_id )
{
    std::vector<CHVertex> processed_nodes;
    std::vector<TNodeContractionCost> node_costs;

    Timer t;

    processed_nodes.reserve( num_vertices( graph ) );
    node_costs.resize( num_vertices( graph ) );

    vector<int> hierarchy_depths( num_vertices( graph ) );

    cout << "Processing initial contraction costs..." << endl;

    // nodes not contracted yet
    TRemainingNodeSet remaining_nodes;

    // Parallel processing of node costs:
    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic)
        for ( int node_i = 0; node_i < int(num_vertices( graph )); node_i++ )
        {
            CHVertex node = CHVertex( node_i );
            node_costs[node] = get_node_cost( graph, node, hierarchy_depths, node_id(node) );
        }
    }

    for ( CHVertex node=0; node < num_vertices( graph ); ++node )
    {
        remaining_nodes.insert( node );
    }

    int num_shortcuts = 0;
    while ( !remaining_nodes.empty() )
    {
        cout << "--------" << endl;
        cout << "Sorting nodes... [processed=" << processed_nodes.size()
             << ", remaining=" << remaining_nodes.size()
             << ", shortcuts=" << num_shortcuts
             << ", elapsed=" << t.elapsed_ms() << "ms]"
             << endl;

        // Sort all remaining nodes by cost
        vector<CHVertex> sorted_nodes(remaining_nodes.begin(), remaining_nodes.end());
        sort(sorted_nodes.begin(), sorted_nodes.end(), [&node_costs](CHVertex a, CHVertex b) { return node_costs[a] < node_costs[b]; });

        // Only contract among the best 20% of remaining nodes during this iteration.
        int step = max(static_cast<size_t>(1), sorted_nodes.size()/5);
        REQUIRE(step <= int(sorted_nodes.size()));

        // Independent node set
        vector<CHVertex> next_nodes = get_independent_node_set(graph, sorted_nodes.cbegin(), step, node_costs );
        REQUIRE(!next_nodes.empty());

        cout << "Contracting " << next_nodes.size()
             << " nodes... [elapsed=" << t.elapsed_ms() << "ms]" << endl;

        // Parallel contractions of selected nodes:
        vector<vector<TEdge>> node_shortcuts(next_nodes.size());
        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic)
            for (int i=0; i<int(next_nodes.size()); ++i)
            {
                node_shortcuts[i] = get_contraction_shortcuts( graph, next_nodes[i] );
            }
        }

        // Update graph after contractions:
        set<CHVertex> impacted_neighbors;
        for (int i=0; i<int(next_nodes.size()); ++i)
        {
            CHVertex nodeID = next_nodes[i];

            apply_on_1_neighbourhood( graph, nodeID, [&impacted_neighbors, &hierarchy_depths, &nodeID] ( CHVertex node ) {
                impacted_neighbors.emplace( node );
                hierarchy_depths[node] = max(hierarchy_depths[nodeID]+1, hierarchy_depths[node]);
                });

            num_shortcuts += static_cast<int>(node_shortcuts[i].size());
            remaining_nodes.erase(nodeID);

            for (const TEdge& shortcut : node_shortcuts[i])
                add_edge_or_update( graph, shortcut.from, shortcut.to, shortcut.cost );

            //graph.removeNode(nodeID);
            clear_vertex( nodeID, graph );
        }

        cout << "Updating " << impacted_neighbors.size()
             << " impacted neighbors. [elapsed=" << t.elapsed_ms() << "ms]" << endl;

        // Parallel update of node costs:
        vector<CHVertex> impacted_neighbors1(impacted_neighbors.begin(), impacted_neighbors.end());
        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic)
            for (int i=0; i<int(impacted_neighbors1.size()); ++i)
            {
                CHVertex update_node = impacted_neighbors1[i];
                node_costs[update_node] = get_node_cost(graph, update_node, hierarchy_depths, node_id( update_node ));
            }
        }

        // Save the order of this contraction (this is our node-ordering).
        processed_nodes.insert(processed_nodes.end(), next_nodes.cbegin(), next_nodes.cend());    // memory already reserved
    }

    cout << "Shortcuts: " << num_shortcuts << endl;
    REQUIRE(processed_nodes.size() == num_vertices( graph ));
    REQUIRE(node_costs.size() == num_vertices( graph ));

    // IDF: 31019ms
    // France: 1237s.

    return processed_nodes;
}

vector<Shortcut> contract_graph( CHGraph& graph )
{
    vector<Shortcut> r;
    Timer t;

    for ( auto node : pair_range( vertices( graph ) ) )
    {
        if(node % 10000 == 0)
            cout << "Contracting nodes " << node << "... [elapsed=" << t.elapsed_ms() << "ms]" << endl;

        // Single node contraction:
        vector<TEdge> shortcuts = get_contraction_shortcuts( graph, node );

        // Update graph after contraction:
        for(const TEdge& edge : shortcuts)
        {
            r.push_back( {edge.from, edge.to, edge.cost, node} );

            // add the shortcut to the graph
            add_edge_or_update( graph, edge.from, edge.to, edge.cost );
        }

#if REDUCE_GRAPH
        // clear any reference to this node
        clear_vertex( node, graph );
#endif
    }
    // IDF: 7289ms

    cout << "Contracted entire graph in " << t.elapsed_ms() << "ms." << endl;
    return r;
}

} // namespace Tempus
