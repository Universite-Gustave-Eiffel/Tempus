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

#ifndef TEMPUS_POI_HH
#define TEMPUS_POI_HH

#include <boost/optional.hpp>
#include "common.hh"
#include "abscissa.hh"
#include "road_graph.hh"

namespace Tempus
{

///
/// refers to the 'poi' DB's table
struct POI : public Base {
    enum PoiType {
        TypeCarPark = 1,
        TypeSharedCarPoint,
        TypeCyclePark,
        TypeSharedCyclePoint,
        TypeUserPOI
    };

    DECLARE_RW_PROPERTY( poi_type, PoiType );
    DECLARE_RW_PROPERTY( name, std::string );

    ///
    /// List of transport modes that can park here
    DECLARE_RW_PROPERTY( parking_transport_modes, std::vector<db_id_t> );

    ///
    /// Test if a parking mode transport is present
    bool has_parking_transport_mode( db_id_t id ) const;

    ///
    /// Link to a road edge
    DECLARE_RW_PROPERTY( road_edge, Road::Edge );

    ///
    /// optional link to the opposite road edge
    DECLARE_RW_PROPERTY( opposite_road_edge, boost::optional<Road::Edge> );

    ///
    /// Number between 0 and 1 : position of the POI on the main road section
    DECLARE_RW_PROPERTY( abscissa_road_section, Abscissa );

    ///
    /// coordinates
    DECLARE_RW_PROPERTY( coordinates, Point3D );
};

}

#endif
