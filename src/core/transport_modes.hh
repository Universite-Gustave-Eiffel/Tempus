/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
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

//
// This file contains declarations of transport modes
//

#ifndef TEMPUS_TRANSPORT_MODES_HH
#define TEMPUS_TRANSPORT_MODES_HH

#include <map>
#include <string>
#include "common.hh"

namespace Tempus
{

///
/// Types of engines
enum TransportModeEngine
{
    EnginePetrol = 1,
    EngineGasoil,
    EngineLPG,
    EngineElectricCar,
    EngineElectricCycle
};

///
/// Traffic rules for transport modes, this is a bitfield value
enum TransportModeTrafficRule
{
    TrafficRulePedestrian      = 1 << 0,
    TrafficRuleBicycle         = 1 << 1,
    TrafficRuleCar             = 1 << 2,
    TrafficRuleTaxi            = 1 << 3,
    TrafficRuleCarPool         = 1 << 4,
    TrafficRuleTruck           = 1 << 5,
    TrafficRuleCoach           = 1 << 6,
    /// Special traffic rule for public transport
    TrafficRulePublicTransport = 1 << 16
};

///
/// Transport mode speed rule
enum TransportModeSpeedRule
{
    SpeedRulePedestrian = 1,
    SpeedRuleBicycle,
    SpeedRuleElectricCycle,
    SpeedRuleRollerSkate,
    SpeedRuleCar,
    SpeedRuleTruck
};

///
/// Transport mode tool pricing rule, bitfield value
enum TransportModeTollRule
{
    TollRuleLightVehicule        = 1,
    TollRuleIntermediateVehicule = 2,
    TollRule2Axles               = 4, ///<- Trucks and coaches with 2 axles
    TollRule3Axles               = 8, ///<- Trucks and coaches with 3 or more axles
    TollRuleMotorcycles          = 16
};

/// GTFS Route type (for public transport)
enum TransportModeRouteType {
    RouteTypeTram = 1,
    RouteTypeSubway,
    RouteTypeRail,
    RouteTypeBus,
    RouteTypeFerry,
    RouteTypeCableCar,
    RouteTypeSuspendedCar,
    RouteTypeFunicular
};

class TransportMode : public Base
{
    /// name of the transport mode
    DECLARE_RW_PROPERTY( name, std::string );
    /// is it a public transport ?
    DECLARE_RW_PROPERTY( is_public_transport, bool );

    /// does it need a parking ?
    DECLARE_RW_PROPERTY( need_parking, bool );

    /// is it shared
    DECLARE_RW_PROPERTY( is_shared, bool );

    /// is it shared and must be returned to its initial station ?
    DECLARE_RW_PROPERTY( must_be_returned, bool );

    /// get traffic rules
    unsigned traffic_rules() const;
    /// set traffic rules
    void set_traffic_rules( unsigned rules );

    DECLARE_RW_PROPERTY( speed_rule, TransportModeSpeedRule );

    DECLARE_RW_PROPERTY( toll_rules, unsigned );

    DECLARE_RW_PROPERTY( engine_type, TransportModeEngine );

    DECLARE_RW_PROPERTY( route_type, TransportModeRouteType );
private:
    unsigned traffic_rules_;
};

///
/// Database id used for common transport modes
enum TransportModeId
{
    TransportModeWalking = 1,
    TransportModePrivateBicycle = 2,
    TransportModePrivateCar = 3,
    TransportModeTaxi = 4
};

}

#endif
