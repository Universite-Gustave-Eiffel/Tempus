#include <string>
#include <unordered_set>

#include <boost/property_map/function_property_map.hpp>

#include "contraction.hh"
#include "common.hh"
#include "variant.hh"
#include "routing_data.hh"
#include "multimodal_graph.hh"
#include "db.hh"

using TNodeContractionCost = int;
using TRemainingNodeSet = std::unordered_set<CHVertex>;

using namespace std;

void get_node_edge_impact( const CHGraph& graph, CHVertex v, int& nb_added_edges, int& max_search_space )
{
    // Contraction of 'node'.

    nb_added_edges = 0;
    max_search_space = 0;

    for ( auto uv : pair_range( in_edges( v, graph ) ) )
        //    if(!successors.empty() && !predecessors.empty() && !(successors.size() == 1 && predecessors.size() == 1 && successors.back().node == predecessors.back().node))
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

#if 0
void getNodeEdgeImpact(const Graph& graph, CHVertex node, int& nbAddedEdges, int& maxSearchSpace)
{
    const vector<Neighbor>& predecessors = graph.predecessors[node];
    const vector<Neighbor>& successors = graph.successors[node];

    maxSearchSpace = 0;
    nbAddedEdges = 0;

    // If there is no successor or no predecessor, then no shortcut will be added.
    if (successors.empty() || predecessors.empty())
        return;

    // If there is only 1 neighbor which is both successor and predecessor,
    // then no shortcut will be added.
    if (successors.size() == 1 && predecessors.size() == 1 && successors.back().node == predecessors.back().node)
        return;

    for (const Neighbor& predecessor : predecessors)
    {
        TTargetSet targets;
        TCost maxSuccessorCost = 0;
        for (const Neighbor& successor : successors)
        {
            if (successor.node != predecessor.node)
            {
                targets.insert(successor.node);
                if(successor.cost > maxSuccessorCost)
                    maxSuccessorCost = successor.cost;
            }
        }

        if (!targets.empty())
        {
            // No shortcut can be longer than w(p, node) + max(w(node, s))
            TCost cutoff = predecessor.cost + maxSuccessorCost;

            // Perform Dijkstra from 'predecessor' to all 'targets' while ignoring 'node'
            int searchSpace = 0;
            auto targetCosts = getDijkstraShortcuts(graph, node, predecessor.node, targets, cutoff, searchSpace);

            if (searchSpace > maxSearchSpace)
                maxSearchSpace = searchSpace;

            for (const Neighbor& successor : successors)
            {
                if (successor.node == predecessor.node)
                    continue;

                // Shortcut is added if no shorter path was found
                auto iterShortcut = targetCosts.find(successor.node);
                if (iterShortcut == targetCosts.end() || iterShortcut->second > (predecessor.cost + successor.cost))
                   ++nbAddedEdges;
            }
        }
    }
}
#endif

//TNodeContractionCost getNodeCostTCH(const CHGraph& graph,
//                                    TNodeId node,
//                                    const vector<int>& hierarchyDepths,
//                                    TNodeId64 nodeId64)
TNodeContractionCost get_node_cost( const CHGraph& graph,
                                    CHVertex node,
                                    const vector<int>& hierarchyDepths,
                                    TNodeId64 nodeId64)
{
    //const vector<Neighbor>& predecessors = graph.predecessors[node];
    //const vector<Neighbor>& successors = graph.successors[node];

    int nbRemovedEdges = 0;
    int nbAddedEdges = 0;
    int maxSearchSpace = 0;

    //nbRemovedEdges = int(successors.size() + predecessors.size());
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


#if 0
inline bool isNodeIndependent(const Graph& graph,
                              const vector<Neighbor>& neighbors,
                              const vector<TNodeContractionCost>& nodeCosts,
                              TNodeContractionCost nodeCost)
{
    // A node is 'independent' if all its neighbors, and the neighbors
    // of its neighbors have a higher cost.

    for (const Neighbor& n : neighbors)
    {
        REQUIRE(n.node < nodeCosts.size());
        if (nodeCosts[n.node] < nodeCost)
            return false;

        const vector<Neighbor>& pred = graph.predecessors[n.node];
        for (const Neighbor& p : pred)
            if (nodeCosts[p.node] < nodeCost)
                return false;

        const vector<Neighbor>& succ = graph.successors[n.node];
        for (const Neighbor& s : succ)
            if (nodeCosts[s.node] < nodeCost)
                return false;
    }
    return true;
}
#endif

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


#if 0
vector<CHVertex> getIndependentNodeSet(const Graph& graph,
                                      vector<TNodeId>::const_iterator nodeBeginIt,
                                      int numNodes,
                                      const vector<TNodeContractionCost>& nodeCosts,
                                      const vector<TNodeId64>& nodeIds64)
{
    // Get all independent nodes from the sequence [nodeBeginIt : NodeBeginIt+numNodes]

    REQUIRE(!nodeCosts.empty());
    REQUIRE(numNodes > 0);

    vector<TNodeId> returnValue;
    unordered_set<TNodeId> excluded;
    auto nodeEndIt = nodeBeginIt + numNodes;
    for (auto nodeIt=nodeBeginIt; nodeIt!=nodeEndIt; ++nodeIt)
    {
        TNodeId nodeID = *nodeIt;
        if (excluded.find(nodeID) != excluded.end())
            continue;
        REQUIRE(nodeID < nodeCosts.size());
        TNodeContractionCost nodeCost = nodeCosts[nodeID];

        const vector<Neighbor>& predecessors = graph.predecessors[nodeID];
        if (!isNodeIndependent(graph, predecessors, nodeCosts, nodeCost))
            continue;

        const vector<Neighbor>& successors = graph.successors[nodeID];
        if (!isNodeIndependent(graph, successors, nodeCosts, nodeCost))
            continue;

        for (const Neighbor& p : predecessors)
        {
            const vector<Neighbor>& pPred = graph.predecessors[p.node];
            const vector<Neighbor>& pSucc = graph.successors[p.node];
            for (const Neighbor& pp : pPred)
                excluded.emplace(pp.node);
            for (const Neighbor& ps : pSucc)
                excluded.emplace(ps.node);
            excluded.emplace(p.node);
        }

        for (const Neighbor& s : successors)
        {
            const vector<Neighbor>& sPred = graph.predecessors[s.node];
            const vector<Neighbor>& sSucc = graph.successors[s.node];
            for (const Neighbor& sp : sPred)
                excluded.emplace(sp.node);
            for (const Neighbor& ss : sSucc)
                excluded.emplace(ss.node);
            excluded.emplace(s.node);
        }

        returnValue.push_back(nodeID);
    }

    return returnValue;
}
#endif

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


// "Root" function
//void getOrderedNodesTCH(Graph& graph,
//                        vector<TNodeId>& processedNodes,
//                        vector<TNodeContractionCost>& nodeCosts,
//                        const vector<TNodeId64>& nodeIds64)
void get_ordered_nodes( CHGraph& graph,
                        vector<CHVertex>& processedNodes,
                        vector<TNodeContractionCost>& nodeCosts,
                        const std::function<TNodeId64(CHVertex)>& node_id )
{
    // preconditions : these two variables are only to return
    REQUIRE(processedNodes.size()==0);
    REQUIRE(nodeCosts.size()==0);

    chrono::milliseconds t = mstime();

    //processedNodes.reserve(graph.maxNode);
    processedNodes.reserve( num_vertices( graph ) );
    //nodeCosts.resize(graph.maxNode);
    nodeCosts.resize( num_vertices( graph ) );

    //vector<int> hierarchyDepths(graph.maxNode);
    vector<int> hierarchyDepths( num_vertices( graph ) );

    cout << "Processing initial contraction costs..." << endl;

    // nodes not contracted yet
    TRemainingNodeSet remainingNodes;

    // Parallel processing of node costs:
    //#pragma omp parallel
    {
        //#pragma omp for schedule(dynamic)
        //for (TNodeId node=0; node<graph.maxNode; ++node)
        for ( CHVertex node = 0; node < num_vertices( graph ); node++ )
        {
            nodeCosts[node] = get_node_cost( graph, node, hierarchyDepths, node_id(node) );
        }
    }

    //for (TNodeId node=0; node<graph.maxNode; ++node)
    for ( CHVertex node=0; node < num_vertices( graph ); ++node )
    {
        remainingNodes.insert( node );
    }

    int numShortcuts = 0;
    while ( !remainingNodes.empty() )
    {
        cout << "--------" << endl;
        cout << "Sorting nodes... [processed=" << processedNodes.size()
             << ", remaining=" << remainingNodes.size()
             << ", shortcuts=" << numShortcuts
             << ", elapsed=" << (mstime() - t).count() << "ms]"
             << endl;

        // Sort all remaining nodes by cost
        vector<CHVertex> sortedNodes(remainingNodes.begin(), remainingNodes.end());
        sort(sortedNodes.begin(), sortedNodes.end(), [&nodeCosts](CHVertex a, CHVertex b) { return nodeCosts[a] < nodeCosts[b]; });

        // Only contract among the best 20% of remaining nodes during this iteration.
        int step = max(static_cast<size_t>(1), sortedNodes.size()/5);
        REQUIRE(step <= int(sortedNodes.size()));

        // Independent node set
        // --------------------
        // ** Could be optimized in the following way: **
        // RemainingNodes and nextNodes can be made of only one vector of pairs (NodeId, Independance_status).
        // The routine work with iterators (in the first iteration, begin() and (end)).
        // When all the independence status are set in the routine, the vector is sorted according the status,
        // so that the selected nodes are grouped together at the end. Then we get two pairs of iterator :
        // begin and end of each group.
        // The routine is applied again to the first group.
        // So we allocate only one vector for all the process.
        vector<CHVertex> nextNodes = get_independent_node_set(graph, sortedNodes.cbegin(), step, nodeCosts );
        REQUIRE(!nextNodes.empty());

        cout << "Contracting " << nextNodes.size()
             << " nodes... [elapsed=" << (mstime() - t).count() << "ms]" << endl;

        // Parallel contractions of selected nodes:
        vector<vector<TEdge>> nodeShortcuts(nextNodes.size());
        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic)
            for (int i=0; i<int(nextNodes.size()); ++i)
            {
                nodeShortcuts[i] = get_contraction_shortcuts( graph, nextNodes[i] );
            }
        }

        // Update graph after contractions:
        set<CHVertex> impactedNeighbors;
        for (int i=0; i<int(nextNodes.size()); ++i)
        {
            CHVertex nodeID = nextNodes[i];

            apply_on_1_neighbourhood( graph, nodeID, [&impactedNeighbors, &hierarchyDepths, &nodeID] ( CHVertex node ) {
                impactedNeighbors.emplace( node );
                hierarchyDepths[node] = max(hierarchyDepths[nodeID]+1, hierarchyDepths[node]);
                });
#if 0
            const vector<Neighbor>& predecessors = graph.predecessors[nodeID];
            for (const Neighbor& p : predecessors)
            {
                impactedNeighbors.emplace(p.node);
                hierarchyDepths[p.node] = max(hierarchyDepths[nodeID]+1, hierarchyDepths[p.node]);
            }

            const vector<Neighbor>& successors = graph.successors[nodeID];
            for (const Neighbor& s : successors)
            {
                impactedNeighbors.emplace(s.node);
                hierarchyDepths[s.node] = max(hierarchyDepths[nodeID]+1, hierarchyDepths[s.node]);
            }
#endif

            numShortcuts += static_cast<int>(nodeShortcuts[i].size());
            remainingNodes.erase(nodeID);

            for (const TEdge& shortcut : nodeShortcuts[i])
                add_edge_or_update( graph, shortcut.from, shortcut.to, shortcut.cost );

            //graph.removeNode(nodeID);
            clear_vertex( nodeID, graph );
        }

        cout << "Updating " << impactedNeighbors.size()
             << " impacted neighbors. [elapsed=" << (mstime() - t).count() << "ms]" << endl;

        // Parallel update of node costs:
        vector<CHVertex> impactedNeighbors1(impactedNeighbors.begin(), impactedNeighbors.end());
        //#pragma omp parallel
        {
            //#pragma omp for schedule(dynamic)
            for (int i=0; i<int(impactedNeighbors1.size()); ++i)
            {
                CHVertex updateNode = impactedNeighbors1[i];
                nodeCosts[updateNode] = get_node_cost(graph, updateNode, hierarchyDepths, node_id( updateNode ));
            }
        }

        // Save the order of this contraction (this is our node-ordering).
        processedNodes.insert(processedNodes.end(), nextNodes.cbegin(), nextNodes.cend());    // memory already reserved
    }

    cout << "Shortcuts: " << numShortcuts << endl;
    REQUIRE(processedNodes.size() == num_vertices( graph ));
    REQUIRE(nodeCosts.size() == num_vertices( graph ));

    // IDF: 31019ms
    // France: 1237s.
}

int main( int argc, char *argv[] )
{
    using namespace Tempus;

    const std::string& dbstring = argc > 1 ? argv[1] : "dbname=tempus_test_db";

    TextProgression progression;
    VariantMap options;
    options["db/options"] = Variant::from_string( dbstring );
    const RoutingData* data = load_routing_data( "multimodal_graph", progression, options );

    const Multimodal::Graph& graph = *dynamic_cast<const Multimodal::Graph*>(data);

    const Road::Graph& road_graph = graph.road();

    CHGraph ch_graph;

    // copy to ch_graph
    for ( Road::Vertex v : pair_range( vertices( road_graph ) ) ) {
            CHVertex new_v = add_vertex( ch_graph );
            ch_graph[new_v].id = road_graph[v].db_id();
    }

    for ( Road::Edge e : pair_range( edges( road_graph )) ) {
            if ( (road_graph[e].traffic_rules() & TrafficRulePedestrian) == 0 ) {
                continue;
            }

            CHVertex v1 = source( e, road_graph );
            CHVertex v2 = target( e, road_graph );
            bool added = false;
            CHEdge new_e;
            boost::tie( new_e, added ) = add_edge( v1, v2, ch_graph );

            BOOST_ASSERT( added );
            ch_graph[new_e].weight = int(road_graph[e].length() * 100.0);
    }

    std::vector<CHVertex> ordered_nodes;
    std::vector<TNodeContractionCost> node_costs;
    get_ordered_nodes( ch_graph, ordered_nodes, node_costs, [&ch_graph](CHVertex v){ return ch_graph[v].id; } );


    Db::Connection conn( dbstring );
    conn.exec( "CREATE SCHEMA IF NOT EXISTS ch2" );
    conn.exec( "DROP TABLE IF EXISTS ch2.ordered_nodes CASCADE" );
    conn.exec( "CREATE TABLE ch2.ordered_nodes (id SERIAL PRIMARY KEY, node_id BIGINT NOT NULL, sort_order INT NOT NULL)" );

    conn.exec( "BEGIN" );
    uint32_t i = 0;
    for ( CHVertex v : ordered_nodes )
    {
            std::ostringstream ostr;
            ostr << "INSERT INTO ch2.ordered_nodes (node_id, sort_order) VALUES (" << ch_graph[v].id << "," << i << ")";
            conn.exec( ostr.str() );
            i++;
    }
    conn.exec( "COMMIT" );
}
