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

#ifndef TEMPUS_CH_ROUTING_DATA_HH
#define TEMPUS_CH_ROUTING_DATA_HH

#include <vector>

#include "ch_query_graph.hh"
#include "routing_data.hh"
#include "routing_data_builder.hh"
#include "serializers.hh"

namespace Tempus
{

struct CHEdgeProperty
{
    union {
        struct {
            uint32_t cost        :31;
            uint32_t is_shortcut :1;
        } b;
        uint32_t data;
    };
    db_id_t db_id;

    void serialize( std::ostream& ostr, binary_serialization_t t ) const
    {
        Tempus::serialize( ostr, data, t );
        Tempus::serialize( ostr, db_id, t );
    }
    void unserialize( std::istream& istr, binary_serialization_t t )
    {
        Tempus::unserialize( istr, data, t );
        Tempus::unserialize( istr, db_id, t );
    }
};

inline std::ostream& operator<<( std::ostream& ostr, const CHEdgeProperty& e )
{
    ostr << "cost: " << e.b.cost << " shortcut: " << e.b.is_shortcut;
    return ostr;
}

using CHQuery = CHQueryGraph<CHEdgeProperty>;

using CHVertex = uint32_t;
using CHEdge = CHQueryGraph<CHEdgeProperty>::edge_descriptor;

///
/// storage of the "middle" node, i.e. the node involved
/// in a shortcut
using MiddleNodeMap = std::map<std::pair<CHVertex, CHVertex>, CHVertex>;

///
/// Routing data out of a CH query graph
class CHRoutingData : public RoutingData
{
public:
    CHRoutingData( std::unique_ptr<CHQuery> ch_query, MiddleNodeMap&& middle_node, std::vector<db_id_t>&& node_id);

    boost::optional<CHVertex> vertex_from_id( db_id_t id ) const;

    db_id_t vertex_id( CHVertex ) const;

    const CHQuery& ch_query() const { return *ch_query_; }

    const MiddleNodeMap& middle_node() const { return middle_node_; }

private:
    // the CH graph
    std::unique_ptr<CHQuery> ch_query_;

    // middle node of a shortcut
    MiddleNodeMap middle_node_;

    // node index -> node id
    std::vector<db_id_t> node_id_;

    // node id -> index
    std::map<db_id_t, size_t> rnode_id_;

    friend class CHRoutingDataBuilder;
};

class CHRoutingDataBuilder : public RoutingDataBuilder
{
public:
    CHRoutingDataBuilder() : RoutingDataBuilder( "ch_graph" ) {}

    virtual std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback&, const VariantMap& options = VariantMap() ) const override;

    virtual std::unique_ptr<RoutingData> file_import( const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;
    virtual void file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;

    uint32_t version() const override { return 1; }
};

} // namespace Tempus

#endif
