/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *
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

#include "plugin.hh"

#include "ch_query_graph.hh"

namespace Tempus
{

struct CHEdgeProperty
{
    uint32_t cost        :31;
    uint32_t is_shortcut :1;
    db_id_t db_id;
};

using CHQuery = CHQueryGraph<CHEdgeProperty>;

using CHVertex = uint32_t;
using CHEdge = CHQueryGraph<CHEdgeProperty>::edge_descriptor;

///
/// storage of the "middle" node, i.e. the node involved
/// in a shortcut
using MiddleNodeMap = std::map<std::pair<CHVertex, CHVertex>, CHVertex>;

struct CHPluginStaticData
{
    // middle node of a shortcut
    MiddleNodeMap middle_node;

    // the CH graph
    std::unique_ptr<CHQuery> ch_query;

    // node index -> node id
    std::vector<db_id_t> node_id;
};

class CHPlugin : public Plugin {
public:

    static const OptionDescriptionList option_descriptions();
    static const PluginCapabilities plugin_capabilities();

    CHPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~CHPlugin() {
    }

    static void post_build();
    virtual void pre_process( Request& request );
    virtual void process();

private:
    static CHPluginStaticData s_;
};


} // namespace Tempus
