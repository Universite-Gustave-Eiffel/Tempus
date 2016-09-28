#ifndef OSM2TEMPUS_GEOM_H
#define OSM2TEMPUS_GEOM_H

#include "osm2tempus.h"

#include <string>
#include <vector>

std::string linestring_to_ewkb( const std::vector<Point>& points );
std::string point_to_ewkb( float lat, float lon );

///
/// Compute the (signed) angle (in degrees) between three points
float angle_3_points( float ax, float ay, float bx, float by, float cx, float cy );

///
/// Haversine distance
double distance_haversine( double lon1, double lat1, double lon2, double lat2 );

///
/// Haversine distance approximation. OK for road lengths
double distance_spherical_law( double lon1, double lat1, double lon2, double lat2 );

///
/// Length of a linestring
double linestring_length( const std::vector<Point>& points );

#endif
