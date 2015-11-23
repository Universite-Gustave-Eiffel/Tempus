#include <string>

#include <chrono>
#include <unordered_set>

#include <boost/heap/d_ary_heap.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/property_map/function_property_map.hpp>

#include "application.hh"
#include "contraction.hh"

using namespace std;

#define REDUCE_GRAPH 1

vector<Shortcut> contract_graph( CHGraph& graph )
{
    cout << "contract_graph" << endl;

    vector<Shortcut> r;
    chrono::milliseconds t = mstime();

    for ( auto node : pair_range( vertices( graph ) ) )
    {
        if(node % 10000 == 0)
            cout << "Contracting nodes " << node << "... [elapsed=" << (mstime() - t).count() << "ms]" << endl;

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

    cout << "Contracted entire graph in " << (mstime() - t).count() << "ms." << endl;
    return r;
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

    // get nodes, ordered
    std::vector<db_id_t> order_id; // order -> id
    std::map<db_id_t, uint32_t> id_order_map; // id -> order
    Db::Connection conn( dbstring );
    {
        Db::ResultIterator res_it = conn.exec_it( "select node_id from ch2.ordered_nodes order by id asc" );
        Db::ResultIterator it_end;

        CHVertex i = 0;
        for ( ; res_it != it_end; res_it++, i++ ) {
            Db::RowValue res_i = *res_it;
            db_id_t db_id = res_i[0];
            order_id.push_back( db_id );
            //std::cout << i << " -> " << db_id << std::endl;
            id_order_map[db_id] = i;
        }
    }

    std::map<db_id_t, Road::Vertex> id_vertex_map; // id -> vertex
    for ( auto it = vertices( road_graph ).first; it != vertices( road_graph ).second; it++ ) {
        add_vertex( ch_graph );
        id_vertex_map[road_graph[*it].db_id()] = *it;
    }
    for ( CHVertex i = 0; i < order_id.size(); i++ ) {
        db_id_t id = order_id[i];
        ch_graph[i].id = id;
    }

    conn.exec( "CREATE SCHEMA IF NOT EXISTS ch2" );
    conn.exec( "DROP TABLE IF EXISTS ch2.query_graph CASCADE" );
    conn.exec( "CREATE TABLE ch2.query_graph (id SERIAL PRIMARY KEY, node_inf BIGINT NOT NULL, node_sup BIGINT NOT NULL, contracted_id BIGINT, weight INT NOT NULL, constraints INT NOT NULL)" );

    conn.exec( "BEGIN" );
    for ( uint32_t order = 0; order < order_id.size(); order++ ) {
        Road::Vertex v = id_vertex_map[order_id[order]];
        BOOST_ASSERT( ch_graph[order].id == order_id[order] );
        for ( auto it = out_edges( v, road_graph ).first; it != out_edges( v, road_graph ).second; it++ ) {
            if ( (road_graph[*it].traffic_rules() & TrafficRulePedestrian) == 0 ) {
                continue;
            }
            Road::Vertex s = target( *it, road_graph );
            db_id_t id = road_graph[s].db_id();
            //tgraph.addEdge( order, node_id_order[id], int(road_graph[*it].length()*100.0) );
            bool added = false;
            CHEdge e;
            boost::tie( e, added ) = add_edge( order, id_order_map[id], ch_graph );
            BOOST_ASSERT( added );
            ch_graph[e].weight = std::max(int(road_graph[*it].length()*100.0),1);
            //std::cout << order << "->" << id_order_map[id] << " " << order_id[order] << "->" << id << std::endl;
            {
                std::ostringstream ss;
                if ( order < id_order_map[id] ) {
                    ss << "(" << order_id[order] << "," << id << "," << ch_graph[e].weight << ",1)";
                } else {
                    ss << "(" << id << "," << order_id[order] << "," << ch_graph[e].weight << ",2)";
                }
                conn.exec( "INSERT INTO ch2.query_graph (node_inf, node_sup, weight, constraints) VALUES " + ss.str() );
            }
        }
    }
    conn.exec( "COMMIT" );

    std::vector<Shortcut> shortcuts = contract_graph( ch_graph );
    std::cout << "# shortcuts" << shortcuts.size() << std::endl;

    // write shortcuts
    conn.exec( "BEGIN" );
    for ( const Shortcut& s : shortcuts )
    {
        std::ostringstream ss;
        if ( s.from < s.to ) {
            ss << "(" << ch_graph[s.from].id << "," << ch_graph[s.to].id << "," << ch_graph[s.contracted].id << "," << s.cost << ",1)";
        } else {
            ss << "(" << ch_graph[s.to].id << "," << ch_graph[s.from].id << "," << ch_graph[s.contracted].id << "," << s.cost << ",2)";
        }
        conn.exec( "INSERT INTO ch2.query_graph (node_inf, node_sup, contracted_id, weight, constraints) VALUES " + ss.str() );
    }
    conn.exec( "COMMIT" );
}
