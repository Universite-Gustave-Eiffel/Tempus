/**
 *   Copyright (C) 2016 Oslandia <infos@oslandia.com>
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

#ifndef TEMPUS_PLUGIN_ISOCHRONE_H
#define TEMPUS_PLUGIN_ISOCHRONE_H

#include "plugin.hh"

namespace Tempus {
namespace IsochronePlugin {

///
/// Plugin that computes a (multimodal) isochrone
class IsochronePlugin : public Plugin
{
public:
    ///
    /// Constructor of the plugin, called by the core with plugin options
    IsochronePlugin( ProgressionCallback& progression, const VariantMap& options );

    ///
    /// Options of the plugin
    static Plugin::OptionDescriptionList option_descriptions()
    {
        Plugin::OptionDescriptionList odl = Plugin::common_option_descriptions();
        odl.declare_option( "Isochrone/limit", "Limit value of the optimized cost", Variant::from_float(10.0));
        odl.declare_option( "Time/min_transfer_time", "Minimum time necessary for a transfer to be done (in min)", Variant::from_int(2));
        odl.declare_option( "Time/walking_speed", "Average walking speed (km/h)", Variant::from_float(3.6));
        odl.declare_option( "Time/cycling_speed", "Average cycling speed (km/h)", Variant::from_int(12));
        odl.declare_option( "Time/car_parking_search_time", "Car parking search time (min)", Variant::from_int(5));
	odl.declare_option( "Debug/verbose", "Verbose general processing", Variant::from_bool(false));
        return odl;
    }

    ///
    /// Capabilities of the plugin
    static Plugin::Capabilities plugin_capabilities()
    {
        Plugin::Capabilities caps;
        caps.optimization_criteria().push_back( CostId::CostDuration );
        caps.set_depart_after( true );
        // Not yet supported
        // caps.set_arrive_before( true );
        return caps;
    }

    ///
    /// Access to the underlying routing data (graph + auxliary data)
    const RoutingData* routing_data() const override { return graph_; }

    ///
    /// Method the creates a PluginRequest object to handle the request
    virtual std::unique_ptr<PluginRequest> request( const VariantMap& options = VariantMap() ) const override;

private:
    const Multimodal::Graph* graph_;
};

///
/// Class that handles the isochrone request
class IsochronePluginRequest : public PluginRequest
{
public:
    ///
    /// Constructor
    IsochronePluginRequest( const IsochronePlugin* plugin, const VariantMap& options, const Multimodal::Graph* );

    ///
    /// The main processing method
    virtual std::unique_ptr<Result> process( const Request& request ) override;

private:
    const Multimodal::Graph* graph_;
};

} // namespace Isochrone
} // namespace Tempus

#endif
