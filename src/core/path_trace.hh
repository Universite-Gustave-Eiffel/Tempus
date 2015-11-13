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

#ifndef TEMPUS_PATH_TRACE_HH
#define TEMPUS_PATH_TRACE_HH

#include <vector>
#include "routing_data.hh"
#include "variant.hh"

namespace Tempus {

///
/// A ValuedEdge is a MMEdge with some arbitrary values attached to it
class ValuedEdge : public MMEdge
{
public:
    ValuedEdge( const MMVertex& v1, const MMVertex& v2 ) : MMEdge( v1, v2 ) {}
    ValuedEdge( MMVertex&& v1, MMVertex&& v2 ) : MMEdge( v1, v2 ) {}
    ///
    /// Map of arbitrary values. It can be costs or other user-defined values
    DECLARE_RO_PROPERTY( values, VariantMap );

    void set_value( const std::string& key, Variant value ) {
        values_[key] = value;
    }

    /// Geometry of the edge, described as a WKB, for visualization purpose
    /// May be empty.
    DECLARE_RW_PROPERTY( geometry_wkb, std::string );
};

///
/// A path trace is a list of edges with some arbitrary values attached to them
typedef std::vector<ValuedEdge> PathTrace;

} // Tempus

#endif
