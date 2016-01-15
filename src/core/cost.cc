/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
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

#include <string>
#include <ostream>
#include "cost.hh"

namespace Tempus
{
std::ostream& operator<<( std::ostream& out, CostId e )
{
    return (out << int(e));
}

std::string cost_name( CostId cost )
{
    switch ( cost ) {
    case CostId::CostDistance:
        return "distance";
    case CostId::CostDuration:
        return "duration";
    case CostId::CostPrice:
        return "price";
    case CostId::CostCarbon:
        return "carbon";
    case CostId::CostCalories:
        return "calories";
    case CostId::CostNumberOfChanges:
        return "number of changes";
    case CostId::CostVariability:
        return "variability";
    case CostId::CostPathComplexity:
        return "complexity";
    case CostId::CostElevation:
        return "elevation";
    case CostId::CostSecurity:
        return "security";
    case CostId::CostLandmark:
        return "landmark";
    }

    return "--";
}

std::string cost_unit( CostId cost )
{
    switch ( cost ) {
    case CostId::CostDistance:
        return "m";
    case CostId::CostDuration:
        return "min";
    case CostId::CostPrice:
        return "€";
    case CostId::CostCarbon:
        return "g/m^3";
    case CostId::CostCalories:
        return "kcal";
    case CostId::CostElevation:
        return "m";
    default:
        return "";
    }
}
}
