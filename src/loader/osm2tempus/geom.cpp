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

float angle_3_points( float ax, float ay, float bx, float by, float cx, float cy )
{
    float abx = bx - ax; float aby = by - ay;
    float cbx = bx - cx; float cby = by - cy;

    float dot = (abx * cbx + aby * cby); // dot product
    float cross = (abx * cby - aby * cbx); // cross product

    return atan2(cross, dot) / M_PI * 180.;
}

double distance_haversine( double lon1, double lat1, double lon2, double lat2 )
{
    const double R = 6371e3; // earth mean radius in meters
    double rlat1 = lat1 / 180.0 * M_PI;
    double rlat2 = lat2 / 180.0 * M_PI;
    double dlat = (lat2 - lat1) / 180.0 * M_PI / 2;
    double dlon = (lon2 - lon1) / 180.0 * M_PI / 2;
    double a = sin( dlat ) * sin( dlat )
        + cos( rlat1 ) * cos( rlat2 )
        * sin( dlon ) * sin( dlon );
    return R * 2 * atan2( sqrt( a ), sqrt( 1 - a ) );
}

double distance_spherical_law( double lon1, double lat1, double lon2, double lat2 )
{
    const double R = 6371e3; // earth mean radius in meters
    lat1 = lat1 / 180.0 * M_PI;
    lat2 = lat2 / 180.0 * M_PI;
    double dlon = (lon2 - lon1) / 180.0 * M_PI;
    return acos( sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(dlon) ) * R;
}

double linestring_length( const std::vector<Point>& points )
{
    double length = 0;
    for ( size_t i = 0; i < points.size() - 1; i++ ) {
        length += distance_spherical_law( points[i].lon(), points[i].lat(), points[i+1].lon(), points[i+1].lat() );
    }
    return length;
}
