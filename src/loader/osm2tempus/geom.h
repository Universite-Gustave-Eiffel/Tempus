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

#endif
