struct VertexProperty
{
    uint32_t order;
    db_id_t id;
};

struct EdgeProperty
{
    double weight;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> CHGraph;
typedef typename boost::graph_traits<CHGraph>::vertex_descriptor CHVertex;
typedef typename boost::graph_traits<CHGraph>::edge_descriptor CHEdge;

typedef std::map<std::pair<CHVertex, CHVertex>, CHVertex> MiddleNodeMap;

/**
 * Search witness paths for u-v-w from the vertex u to a set of target vertices
 * @param u the predecessor vertex
 * @param v the current vertex to contract
 * @param targets list of successors
 * @param max_cost the maximum cost to consider. If a path longer than that is found, shortcut
 * @returns a vector of shortest path costs for each u-v-target[i]
 */
std::vector<double> witness_search( const CHGraph& graph, CHVertex u, CHVertex v, const std::vector<CHEdge>& targets, double max_cost = .0 )
{
    // shortests paths to target nodes
    std::vector<double> costs;
    costs.resize( targets.size() );

    std::list<std::pair<CHVertex, size_t>> remaining_targets;
    size_t idx = 0;
    for ( CHEdge e : targets ) {
        CHVertex w = target( e, graph );
        if ( w == u ) {
            // avoid cycles
            idx++;
            continue;
        }
        remaining_targets.push_back( std::make_pair(w, idx) );
        costs[idx] = std::numeric_limits<double>::max();
        idx++;
    }
    
    //
    // one-to-many dijkstra search
    // on the upward graph
    // and excluding the middle node (v)
    // FIXME: use standard boost dijkstra with an upward graph adaptor ?
#if 0
    typedef std::map<CHVertex, double> PotentialMap;
    PotentialMap potential;
    associative_property_map_default_value< PotentialMap > potential_map( potential, std::numeric_limits<double>::max() );
#else
    typedef std::vector<double> PotentialMap;
    PotentialMap potential( num_vertices( graph ), std::numeric_limits<double>::max() );
    typedef double* PotentialPMap;
    PotentialPMap potential_map = &potential[0];
#endif

    auto cmp = boost::make_indirect_cmp( std::greater<double>(), potential_map );

    typedef boost::heap::d_ary_heap< CHVertex, boost::heap::arity<4>, boost::heap::compare< decltype(cmp) >, boost::heap::mutable_<true> > VertexQueue;
    VertexQueue vertex_queue( cmp );

    //std::cout << "witness search " << graph[u].id << " - " << graph[v].id << std::endl;

    vertex_queue.push( u ); 
    put( potential_map, u, 0.0 );

    while ( !vertex_queue.empty() ) {
        CHVertex min_v = vertex_queue.top();
        vertex_queue.pop();
        double min_pi = get( potential_map, min_v );
        //std::cout << "min_v " << graph[min_v].id << std::endl;

        // did we reach a target node ?
        // FIXME optimize
        auto it = std::find_if( remaining_targets.begin(), remaining_targets.end(),
                                [&min_v](const std::pair<CHVertex,size_t>& p)
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
            CHVertex vv = target( *oei, graph );
            //std::cout << graph[vv].id << std::endl;
            // exclude the middle node
            if ( vv == v )
                continue;
            double new_pi = get( potential_map, vv );
            double cost = graph[*oei].weight;
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

void contract_graph( CHGraph& ch_graph, const std::map<CHVertex,CHVertex>& node_map, MiddleNodeMap& middle_node )
{
    size_t nshortcuts = 0;
    std::map<std::pair<CHVertex, CHVertex>, double> shortcuts;

    size_t v_idx = 0;
    for ( auto p : node_map ) {
        // node_map is sorted by ascending order
        v_idx++;
        CHVertex v = p.second;
        //std::cout << "contracting " << v << " - order " << p.first << std::endl;

        // witness search

        std::vector<CHEdge> successors;
        successors.reserve( out_degree( v, ch_graph ) );

        double max_cost = 0.0;
        for ( auto oei = out_edges( v, ch_graph ).first; oei != out_edges( v, ch_graph ).second; oei++ ) {
            auto succ = target( *oei, ch_graph );
            // only retain successors of higher order
            if ( ch_graph[succ].order < ch_graph[v].order )
                continue;
            //std::cout << "succ " << road_graph[succ].db_id() << std::endl;

            successors.push_back( *oei );

            // get the max cost that will be used for the witness search
            if ( ch_graph[*oei].weight > max_cost )
                max_cost = ch_graph[*oei].weight;
        }

        //std::cout << "#successors " << successors.size() << std::endl;
        if ( successors.empty() )
            continue;

        for ( auto iei = in_edges( v, ch_graph ).first; iei != in_edges( v, ch_graph ).second; iei++ ) {
            auto pred = source( *iei, ch_graph );
            // only retain predecessors of higher order
            if ( ch_graph[pred].order < ch_graph[v].order )
                continue;

            double pred_cost = ch_graph[*iei].weight;
            //std::cout << "u " << ch_graph[pred].id << " v " << ch_graph[v].id << " uv cost " << pred_cost << std::endl;

            std::vector<double> witness_costs = witness_search( ch_graph, pred, v, successors, pred_cost + max_cost );
            size_t i = 0;
            for ( auto vw_it = successors.begin(); vw_it != successors.end(); vw_it++, i++ ) {
                CHVertex w = target( *vw_it, ch_graph );
                if ( w == pred ) {
                    continue;
                }
                double uvw_cost = pred_cost + ch_graph[*vw_it].weight;
                if ( witness_costs[i] > uvw_cost ) {
                    // need to add a shortcut between pred and succ
                    shortcuts[std::make_pair(pred, w)] = uvw_cost;
                    if ( v_idx % 100 == 0 ) {
                        //float p = v_idx / float(num_vertices(ch_graph));
                        //std::cout << v_idx << " " << p << std::endl;
                    }

                    middle_node[std::make_pair(pred, w)] = v;
                }
            }
        }

        for ( auto shortcut : shortcuts ) {
            auto p = shortcut.first;
            double cost = shortcut.second;
            bool found;
            CHEdge e;
            // update the cost or add a new shortcut
            std::tie(e, found) = edge( p.first, p.second, ch_graph );
            if ( found ) {
                BOOST_ASSERT( ch_graph[e].weight >= cost );
                ch_graph[e].weight = cost;
            }
            else {
                std::tie(e, found) = add_edge( p.first, p.second, ch_graph );
                nshortcuts++;
                BOOST_ASSERT( found );
                ch_graph[e].weight = cost;
            }
        }
    }

#if 0
    for ( auto shortcut : shortcuts ) {
        auto p = shortcut.first;
        std::cout << "SHORTCUT " << road_graph[p.first].db_id() << "," << road_graph[p.second].db_id() << ","
                  << road_graph[spriv->middle_node[p]].db_id() << ","
                  << shortcut.second
                  << std::endl;
    }
#endif


    std::cout << num_vertices( ch_graph ) << " " << num_edges( ch_graph ) << std::endl;
    std::cout << "# shortcuts " << nshortcuts << std::endl;
}

