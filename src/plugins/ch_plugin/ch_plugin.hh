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
#include "ch_routing_data.hh"

namespace Tempus
{

class CHPlugin : public Plugin
{
public:

    static const OptionDescriptionList option_descriptions();
    static const Capabilities plugin_capabilities();

    CHPlugin( ProgressionCallback& progression, const VariantMap& options );

    const RoutingData* routing_data() const override { return rd_; }

    std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const override;

private:
    const CHRoutingData* rd_;
};

} // namespace Tempus
