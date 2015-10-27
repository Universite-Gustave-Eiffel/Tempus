#include "contraction.hh"

#include "common.hh"

#include <queue>

using namespace std;

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
