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
    DECLARE_RW_PROPERTY( x, float );
    DECLARE_RW_PROPERTY( y, float );
public:
    Point2D() : x_( 0.0 ), y_( 0.0 ) {}
    Point2D( float mx, float my ) : x_( mx ), y_( my ) {}
};

///
/// 3D Points
struct Point3D {
    DECLARE_RW_PROPERTY( x, float );
    DECLARE_RW_PROPERTY( y, float );
    DECLARE_RW_PROPERTY( z, float );
public:
    Point3D() : x_( 0.0 ), y_( 0.0 ), z_( 0.0 ) {}
    Point3D( float mx, float my, float mz ) : x_( mx ), y_( my ), z_( mz ) {}
};

/** Compute the distance between a and b */
float distance( const Point3D& a, const Point3D& b );
/** Compute the distance between a and b */
float distance( const Point2D& a, const Point2D& b );

/** Compute the square of the distance between a and b */
float distance2( const Point3D& a, const Point3D& b );
/** Compute the square of the distance between a and b */
float distance2( const Point2D& a, const Point2D& b );

}

#endif
