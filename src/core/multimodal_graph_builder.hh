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

#include "routing_data_builder.hh"
#include "road_graph.hh"
#include "db.hh"

namespace Tempus
{

class MultimodalGraphBuilder : public RoutingDataBuilder
{
public:
    MultimodalGraphBuilder() : RoutingDataBuilder( "multimodal_graph" ) {}

    virtual std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback&, const VariantMap& options = VariantMap() ) const override;

    virtual std::unique_ptr<RoutingData> file_import( const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;
    virtual void file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;
};

Road::Restrictions import_turn_restrictions( Db::Connection& connection, const Road::Graph& graph, const std::string& schema_name = "tempus" );

}
