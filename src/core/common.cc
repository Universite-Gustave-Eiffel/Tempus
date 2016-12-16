/**
 *   Copyright (C) 2012-2016 Oslandia <infos@oslandia.com>
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
#include "multimodal_graph_builder.hh"
#include "ch_routing_data.hh"

void tempus_init()
{
    using namespace Tempus;
    if ( RoutingDataBuilderRegistry::instance().builder_list().size() == 0 ) {
        RoutingDataBuilderRegistry::instance().addBuilder( std::unique_ptr<RoutingDataBuilder>( new MultimodalGraphBuilder() ) );
        RoutingDataBuilderRegistry::instance().addBuilder( std::unique_ptr<RoutingDataBuilder>( new CHRoutingDataBuilder() ) );
    }
}
