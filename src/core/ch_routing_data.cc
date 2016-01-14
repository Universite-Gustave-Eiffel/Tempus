/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "ch_routing_data.hh"
#include "db.hh"

#include <fstream>
#include <boost/format.hpp>

namespace Tempus
{

CHRoutingData::CHRoutingData( std::unique_ptr<CHQuery> a_ch_query, MiddleNodeMap&& a_middle_node, std::vector<db_id_t>&& a_node_id) :
    RoutingData( "ch_graph" ),
    ch_query_( std::move(a_ch_query) ),
    middle_node_( a_middle_node ),
    node_id_( a_node_id )
{
    // update the reverse id map
    for ( size_t i = 0; i < node_id_.size(); i++ ) {
        rnode_id_[node_id_[i]] = i;
    }
}

boost::optional<CHVertex> CHRoutingData::vertex_from_id( db_id_t id ) const
{
    auto it = rnode_id_.find( id );
    if ( it != rnode_id_.end() ) {
        return it->second;
    }
    return boost::optional<CHVertex>();
}

db_id_t CHRoutingData::vertex_id( CHVertex v ) const
{
    return node_id_[v];
}

std::unique_ptr<RoutingData> CHRoutingDataBuilder::pg_import( const std::string& pg_options, ProgressionCallback&, const VariantMap& options ) const
{
    std::unique_ptr<CHQuery> ch_query;
    MiddleNodeMap middle_node;
    std::vector<db_id_t> node_id;

    std::string schema = "ch";
    if ( options.find( "ch/schema" ) != options.end() ) {
        schema = options.find( "ch/schema" )->second.str();
    }
    std::cout << "schema : " << schema << std::endl;
    
    Db::Connection conn( pg_options );

    size_t num_nodes = 0;
    {
        Db::Result r( conn.exec( (boost::format("select count(*) from %1%.ordered_nodes") % schema).str() ) );
        BOOST_ASSERT( r.size() == 1 );
        r[0][0] >> num_nodes;
    }
    {
        Db::ResultIterator res_it = conn.exec_it( (boost::format( "select * from\n"
                                                                  "(\n"
                                                  // the upward part
                                                  "select o1.sort_order as id1, o2.sort_order as id2, weight, o3.sort_order as mid, 0 as dir, rs1.id as eid1, rs2.id as eid2, o1.node_id, o2.node_id, o3.node_id\n"
                                                  "from %1%.query_graph\n"
                                                  "left join %1%.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  "left join tempus.road_section as rs1 on rs1.node_from = node_inf and rs1.node_to = node_sup\n"
                                                  "left join tempus.road_section as rs2 on rs2.node_from = node_sup and rs2.node_to = node_inf\n"
                                                  ", %1%.ordered_nodes as o1, %1%.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 1 > 0\n"

                                                  // union with the downward part
                                                  "union all\n"
                                                  "select o1.sort_order as id1, o2.sort_order as id2, weight, o3.sort_order as mid, 1 as dir, rs1.id as eid1, rs2.id as eid2, o1.node_id, o2.node_id, o3.node_id\n"
                                                  "from %1%.query_graph\n"
                                                  "left join %1%.ordered_nodes as o3 on o3.node_id = contracted_id\n"
                                                  "left join tempus.road_section as rs1 on rs1.node_from = node_inf and rs1.node_to = node_sup\n"
                                                  "left join tempus.road_section as rs2 on rs2.node_from = node_sup and rs2.node_to = node_inf\n"
                                                  ", %1%.ordered_nodes as o1, %1%.ordered_nodes as o2\n"
                                                  "where o1.node_id = node_inf and o2.node_id = node_sup\n"
                                                  "and query_graph.\"constraints\" & 2 > 0\n"
                                                                 ") t order by id1, dir, id2, weight asc" ) % schema).str()
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
            uint32_t id1 = res_i[0].as<uint32_t>();
            uint32_t id2 = res_i[1].as<uint32_t>();
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
            p.b.cost = res_i[2];
            p.db_id = eid;
            p.b.is_shortcut = 0;
            if ( !res_i[3].is_null() ) {
                uint32_t middle = res_i[3].as<uint32_t>();
                // we have a middle node, it is a shortcut
                p.b.is_shortcut = 1;
                if ( dir == 0 ) {
                    middle_node[std::make_pair(id1, id2)] = middle;
                }
                else {
                    middle_node[std::make_pair(id2, id1)] = middle;
                }
            }
            //std::cout << std::endl;
            properties.emplace_back( p );
            targets.emplace_back( std::make_pair(id1, id2) );
        }
        up_degrees.push_back( upd );

        ch_query.reset( new CHQueryGraph<CHEdgeProperty>( targets.begin(), num_nodes, up_degrees.begin(), properties.begin() ) );
        std::cout << "OK" << std::endl;
    }
    {
        node_id.resize( num_nodes );
        Db::ResultIterator res_it = conn.exec_it( (boost::format("select node_id from %1%.ordered_nodes order by id asc") % schema).str() );
        Db::ResultIterator it_end;
        uint32_t v = 0;
        for ( ; res_it != it_end; res_it++, v++ ) {
            Db::RowValue res_i = *res_it;
            db_id_t id = res_i[0];
            BOOST_ASSERT( v < node_id.size() );
            node_id[v] = id;
        }
    }

    // check consistency
    {
        CHQuery& ch = *ch_query;
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
        CHQuery& ch = *ch_query;
        auto it_end = edges( ch ).second;
        for ( auto it = edges( ch ).first; it != it_end; it++ ) {
            CHVertex u = source( *it, ch );
            CHVertex v = target( *it, ch );
            bool found = false;
            CHEdge e;
            std::tie( e, found ) = edge( u, v, ch );
            BOOST_ASSERT( found );
            BOOST_ASSERT( u == source( e, ch ) );
            BOOST_ASSERT( v == target( e, ch ) );
            BOOST_ASSERT( it->property().b.cost == e.property().b.cost );
        }
    }

    std::unique_ptr<RoutingData> rd( new CHRoutingData( std::move(ch_query), std::move(middle_node), std::move(node_id) ) );

    // import transport modes
    RoutingData::TransportModes all_modes = load_transport_modes( conn );
    RoutingData::TransportModes modes;
    // keep only pedestrian
    modes[TransportModeWalking] = all_modes[TransportModeWalking];
    rd->set_transport_modes( modes );

    return std::move( rd );
}

std::unique_ptr<RoutingData> CHRoutingDataBuilder::file_import( const std::string& filename, ProgressionCallback& /*progression**/, const VariantMap& /*options*/ ) const
{
    std::ifstream ifs( filename );
    if ( ifs.fail() ) {
        throw std::runtime_error( "Problem opening input file " + filename );
    }

    read_header( ifs );

    std::cout << "read graph" << std::endl;
    std::unique_ptr<CHQuery> query( new CHQuery() );
    query->unserialize( ifs, binary_serialization_t() );

    std::cout << "read middle node" << std::endl;
    MiddleNodeMap middle_node;
    unserialize( ifs, middle_node, binary_serialization_t() );

    std::cout << "read node id" << std::endl;
    std::vector<db_id_t> node_id;
    unserialize( ifs, node_id, binary_serialization_t() );

    std::unique_ptr<RoutingData> rd( new CHRoutingData( std::move( query ), std::move( middle_node ), std::move( node_id ) ) );
    return rd;
}

void CHRoutingDataBuilder::file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
    std::ofstream ofs( filename );

    write_header( ofs );

    const CHRoutingData* mrd = static_cast<const CHRoutingData*>( rd );

    // serialize the graph
    mrd->ch_query_->serialize( ofs, binary_serialization_t() );

    // middle node
    serialize( ofs, mrd->middle_node_, binary_serialization_t() );
    serialize( ofs, mrd->node_id_, binary_serialization_t() );
}

REGISTER_BUILDER( CHRoutingDataBuilder )

} // namespace Tempus
