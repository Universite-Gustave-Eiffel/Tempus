#include "geom.h"

#include <cmath>

std::string linestring_to_ewkb( const std::vector<Point>& points )
{
    std::string ewkb;
    ewkb.resize( 1 + 4 /*type*/ + 4 /*srid*/ + 4 /*size*/ + 8 * points.size() * 2 );
    memcpy( &ewkb[0], "\x01"
            "\x02\x00\x00\x20"  // linestring + has srid
            "\xe6\x10\x00\x00", // srid = 4326
            9
            );
    // size
    *reinterpret_cast<uint32_t*>( &ewkb[9] ) = points.size();
    for ( size_t i = 0; i < points.size(); i++ ) {
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 0 ) = points[i].lon();
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 8 ) = points[i].lat();
    }
    return ewkb;
}

std::string point_to_ewkb( float lat, float lon )
{
    std::string ewkb;
    ewkb.resize( 1 + 4 /*type*/ + 4 /*srid*/ + 8 * 2 );
    memcpy( &ewkb[0], "\x01"
            "\x01\x00\x00\x20"  // point + has srid
            "\xe6\x10\x00\x00", // srid = 4326
            9
            );
    // size
    *reinterpret_cast<double*>( &ewkb[9] + 0 ) = lon;
    *reinterpret_cast<double*>( &ewkb[9] + 8 ) = lat;
    return ewkb;
}

///
/// Compute the (signed) angle (in degrees) between three points
float angle_3_points( float ax, float ay, float bx, float by, float cx, float cy )
{
    float abx = bx - ax; float aby = by - ay;
    float cbx = bx - cx; float cby = by - cy;

    float dot = (abx * cbx + aby * cby); // dot product
    float cross = (abx * cby - aby * cbx); // cross product

    return atan2(cross, dot) / M_PI * 180.;
}
