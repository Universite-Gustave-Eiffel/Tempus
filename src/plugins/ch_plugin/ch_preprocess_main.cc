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

#include "ch_preprocess.hh"
#include "routing_data.hh"
#include "multimodal_graph.hh"
#include "db.hh"

#include <string>
#include <boost/program_options.hpp>

int main( int argc, char *argv[] )
{
    using namespace Tempus;
    using namespace std;

    bool compute_node_ordering = true;
    bool load_ordering_from_db = false;

    bool compute_contraction = true;
    bool save_to_db = true;

    std::string db_options = "dbname=tempus_test_db";
    std::string in_schema = "tempus";
    std::string in_file;
    std::string ordering_out_schema = "ch";
    std::string ordering_in_schema = "ch";
    std::string contraction_out_schema = "ch";

    namespace po = boost::program_options;
    po::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help", "produce help message" )
        ( "db,d", po::value<string>(&db_options), "set database connection options" )
        ( "in_schema,s", po::value<string>(&in_schema), "set database schema for the input graph" )
        ( "in_file,L", po::value<string>(&in_file), "set the name of the dump file where the input graph is located" )
        ( "no-ordering,n", "do not compute the ordering" )
        ( "ordering-out-schema", po::value<string>(&ordering_out_schema), "set database schema used for writing the node ordering" )
        ( "no-contraction,N", "do not compute the contraction" )
        ( "ordering-loading", "load the node ordering from the db" )
        ( "ordering-in-schema", po::value<string>(&ordering_in_schema), "set database schema used for reading the node ordering" )
        ( "contraction-out-schema", po::value<string>(&contraction_out_schema), "set database schema used for writing the contraction" )
        ( "no-db-saving", "do not save to db" )
        ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );

    if ( vm.count( "help" ) ) {
        std::cout << desc << std::endl;
        return 1;
    }

    if ( vm.count( "no-ordering" ) ) {
        compute_node_ordering = false;
        load_ordering_from_db = true;
    }
    if ( vm.count( "no-contraction" ) ) {
        compute_contraction = false;
    }
    if ( vm.count( "no-db-saving" ) ) {
        save_to_db = false;
    }
    if ( vm.count( "ordering-loading" ) ) {
        load_ordering_from_db = true;
    }

    TextProgression progression;
    VariantMap options;
    options["db/options"] = Variant::from_string( db_options );
    options["db/schema"] = Variant::from_string( in_schema );
    if ( !in_file.empty() ) {
        options["from_file"] = Variant::from_string( in_file );
    }
    const RoutingData* data = load_routing_data( "multimodal_graph", progression, options );

    const Multimodal::Graph& graph = *dynamic_cast<const Multimodal::Graph*>(data);

    const Road::Graph& road_graph = graph.road();

    std::vector<CHVertex> ordered_nodes;
    //
    // Node ordering
    //
    CHGraph ch_graph;
    if ( compute_node_ordering ) {
        std::cout << "* Computing node ordering" << std::endl;
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

        ordered_nodes = order_graph( ch_graph, [&ch_graph](CHVertex v){ return ch_graph[v].id; } );

        if ( save_to_db ) {
            std::cout << "* Saving node ordering to schema " << ordering_out_schema << std::endl;
            Db::Connection conn( db_options );
            conn.exec( "CREATE SCHEMA IF NOT EXISTS " + ordering_out_schema );
            conn.exec( "DROP TABLE IF EXISTS " + ordering_out_schema + ".ordered_nodes CASCADE" );
            conn.exec( "CREATE TABLE " + ordering_out_schema + ".ordered_nodes (id SERIAL PRIMARY KEY, node_id BIGINT NOT NULL, sort_order INT NOT NULL)" );

            conn.exec( "BEGIN" );
            uint32_t i = 0;
            for ( CHVertex v : ordered_nodes )
            {
                std::ostringstream ostr;
                ostr << "INSERT INTO " << ordering_out_schema << ".ordered_nodes (node_id, sort_order) VALUES (" << ch_graph[v].id << "," << i << ")";
                conn.exec( ostr.str() );
                i++;
            }
            conn.exec( "COMMIT" );
        }
    }

    //
    // Graph contraction
    //
    if ( compute_contraction ) {
        std::cout << "* Compute graph contraction" << std::endl;
        // get nodes, ordered
        std::vector<db_id_t> order_id; // order -> id
        std::map<db_id_t, uint32_t> id_order_map; // id -> order
        Db::Connection conn( db_options );
        if ( load_ordering_from_db ) {
            std::cout << "* Loading node ordering from schema " << ordering_in_schema << std::endl;
            Db::ResultIterator res_it = conn.exec_it( "select node_id from " + ordering_in_schema + ".ordered_nodes order by id asc" );
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
        else {
            for ( uint32_t i = 0; i < ordered_nodes.size(); i++ ) {
                db_id_t id = ch_graph[ordered_nodes[i]].id;
                order_id.push_back( id );
                id_order_map[id] = i;
            }
        }
        // clear the graph from the ordering step
        ch_graph.clear();

        std::map<db_id_t, Road::Vertex> id_vertex_map; // id -> vertex
        for ( auto it = vertices( road_graph ).first; it != vertices( road_graph ).second; it++ ) {
            add_vertex( ch_graph );
            id_vertex_map[road_graph[*it].db_id()] = *it;
        }
        for ( CHVertex i = 0; i < order_id.size(); i++ ) {
            db_id_t id = order_id[i];
            ch_graph[i].id = id;
        }

        if ( save_to_db ) {
            conn.exec( "CREATE SCHEMA IF NOT EXISTS " + contraction_out_schema );
            conn.exec( "DROP TABLE IF EXISTS " + contraction_out_schema + ".query_graph CASCADE" );
            conn.exec( "CREATE TABLE " + contraction_out_schema + ".query_graph (id SERIAL PRIMARY KEY, node_inf BIGINT NOT NULL, node_sup BIGINT NOT NULL, contracted_id BIGINT, weight INT NOT NULL, constraints INT NOT NULL)" );

            conn.exec( "BEGIN" );
        }

        for ( uint32_t order = 0; order < order_id.size(); order++ ) {
            Road::Vertex v = id_vertex_map[order_id[order]];
            BOOST_ASSERT( ch_graph[order].id == order_id[order] );
            for ( auto it = out_edges( v, road_graph ).first; it != out_edges( v, road_graph ).second; it++ ) {
                if ( (road_graph[*it].traffic_rules() & TrafficRulePedestrian) == 0 ) {
                    continue;
                }
                Road::Vertex s = target( *it, road_graph );
                db_id_t id = road_graph[s].db_id();
                bool added = false;
                CHEdge e;
                boost::tie( e, added ) = add_edge( order, id_order_map[id], ch_graph );
                BOOST_ASSERT( added );
                ch_graph[e].weight = std::max(int(road_graph[*it].length()*100.0),1);
                if ( save_to_db ) {
                    std::ostringstream ss;
                    if ( order < id_order_map[id] ) {
                        ss << "(" << order_id[order] << "," << id << "," << ch_graph[e].weight << ",1)";
                    } else {
                        ss << "(" << id << "," << order_id[order] << "," << ch_graph[e].weight << ",2)";
                    }
                    conn.exec( "INSERT INTO " + contraction_out_schema + ".query_graph (node_inf, node_sup, weight, constraints) VALUES " + ss.str() );
                }
            }
        }
        if ( save_to_db ) {
            conn.exec( "COMMIT" );
        }

        std::vector<Shortcut> shortcuts = contract_graph( ch_graph );

        if ( save_to_db ) {
            std::cout << "* Saving contraction to schema " << contraction_out_schema << std::endl;
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
                conn.exec( "INSERT INTO " + contraction_out_schema + ".query_graph (node_inf, node_sup, contracted_id, weight, constraints) VALUES " + ss.str() );
            }
            conn.exec( "COMMIT" );
        }
    }
}
