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

#ifndef TEMPUS_POINT_HH
#define TEMPUS_POINT_HH

#include "common.hh"

namespace Tempus
{


///
/// 2D Points
struct Point2D {
    DECLARE_RW_PROPERTY( x, double );
    DECLARE_RW_PROPERTY( y, double );
};

///
/// 3D Points
struct Point3D {
    DECLARE_RW_PROPERTY( x, double );
    DECLARE_RW_PROPERTY( y, double );
    DECLARE_RW_PROPERTY( z, double );
};

}

#endif
