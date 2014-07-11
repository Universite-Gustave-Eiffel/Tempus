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
#include "cost.hh"

namespace Tempus
{
std::string cost_name( int cost )
{
    switch ( cost ) {
    case CostDistance:
        return "distance";
    case CostDuration:
        return "duration";
    case CostPrice:
        return "price";
    case CostCarbon:
        return "carbon";
    case CostCalories:
        return "calories";
    case CostNumberOfChanges:
        return "number of changes";
    case CostVariability:
        return "variability";
    case CostPathComplexity:
        return "complexity";
    case CostElevation:
        return "elevation";
    case CostSecurity:
        return "security";
    case CostLandmark:
        return "landmark";
    }

    return "--";
}

std::string cost_unit( int cost )
{
    switch ( cost ) {
    case CostDistance:
        return "m";
    case CostDuration:
        return "min";
    case CostPrice:
        return "€";
    case CostCarbon:
        return "g/m^3";
    case CostCalories:
        return "kcal";
    case CostElevation:
        return "m";
    }

    return "";
}
}
