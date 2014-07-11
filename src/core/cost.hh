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

#ifndef TEMPUS_COST_HH
#define TEMPUS_COST_HH

#include <string>
#include <map>

namespace Tempus
{
///
/// Type used to model costs. Either in a Step or as an optimizing criterion.
/// This is a map to a double value and thus is user extensible.
typedef std::map<int, double> Costs;

///
/// Default common cost identifiers
enum CostId {
    CostDistance = 1,
    CostDuration,
    CostPrice,
    CostCarbon,
    CostCalories,
    CostNumberOfChanges,
    CostVariability,
    CostPathComplexity,
    CostElevation,
    CostSecurity,
    CostLandmark,
    FirstValue = CostDistance,
    LastValue = CostLandmark
};

///
/// Returns the name of a cost
/// @param[in] cost The cost identifier. @relates CostId
/// @param[out]     The string representation of the cost
std::string cost_name( int cost );

///
/// Returns the unit of a cost
/// @param[in] cost The cost identifier. @relates CostId
/// @param[out]     The string representation of the cost
std::string cost_unit( int cost );

}

#endif
