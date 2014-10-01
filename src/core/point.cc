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

#include <math.h>
#include "point.hh"

namespace Tempus
{

double distance( const Point2D& a, const Point2D& b )
{
    return sqrt(
                (a.x() - b.x()) * (a.x() - b.x()) +
                (a.y() - b.y()) * (a.y() - b.y()) );
}

double distance( const Point3D& a, const Point3D& b )
{
    return sqrt(
                (a.x() - b.x()) * (a.x() - b.x()) +
                (a.y() - b.y()) * (a.y() - b.y()) +
                (a.z() - b.z()) * (a.z() - b.z()) );
}

}
