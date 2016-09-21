#include "data_profile.h"

#include <algorithm>
#include <math.h>

#if 0
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
#endif

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

class TempusDataProfile : public DataProfile
{
public:
    TempusDataProfile() :
        DataProfile(),
        columns_( { {"length", DataType::Float8Type } } )
    {
    }
    int n_columns() const
    {
        return columns_.size();
    }

    std::vector<Column> columns() const
    {
        return columns_;
    }

    std::vector<DataVariant> section_additional_values( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) const
    {
        std::vector<DataVariant> ret = { linestring_length( points ) };
        return ret;
    }
private:
    std::vector<Column> columns_;
};

DECLARE_DATA_PROFILE(tempus, TempusDataProfile);
