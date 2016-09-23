#include "data_profile.h"

#include "transport_modes.hh"

#include <algorithm>
#include <math.h>

#if 0
///
/// Haversine distance
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

///
/// Haversine distance approximation. OK for road lengths
double distance_spherical_law( double lon1, double lat1, double lon2, double lat2 )
{
    const double R = 6371e3; // earth mean radius in meters
    lat1 = lat1 / 180.0 * M_PI;
    lat2 = lat2 / 180.0 * M_PI;
    double dlon = (lon2 - lon1) / 180.0 * M_PI;
    return acos( sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(dlon) ) * R;
}

///
/// Length of a linestring
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
        columns_( {
                { "osm_id", DataType::UInt64Type },
                { "length", DataType::Float8Type },
                { "traffic_rules_ft", DataType::Int16Type },
                { "traffic_rules_tf", DataType::Int16Type },
                { "car_speed_limit", DataType::Float8Type },
                { "name", DataType::String }
            } ),
        max_speed_( {
                { "motorway", 130.0 },
                { "motorway_link", 130.0 },
                { "trunk", 110.0 },
                { "trunk_link", 110.0 },
                { "primary", 90.0 },
                { "primary_link", 90.0 },
                { "secondary", 90.0 },
                { "tertiary", 90.0 },
                { "residential", 50.0 },
                { "service", 50.0 },
                { "track", 50.0 },
                { "unclassified", 50.0 },
                { "footway", 0.0 } } )
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

    std::vector<DataVariant> section_additional_values( uint64_t way_id, uint64_t /*section_id*/, uint64_t /*node_from*/, uint64_t /*node_to*/, const std::vector<Point>& points, const osm_pbf::Tags& tags ) const
    {
        int16_t traffic_rules_ft = 0, traffic_rules_tf = 0;

        auto oneway_tag = tags.find( "oneway" );
        auto junction_tag = tags.find( "junction" );
        std::string highway_type = tags.find( "highway" )->second;

        // is it a oneway ?
        bool oneway = (oneway_tag != tags.end() && (( oneway_tag->second == "true" || oneway_tag->second == "yes" || oneway_tag->second == "1" ))) ||
            (junction_tag != tags.end() && junction_tag->second == "roundabout") ||
            highway_type == "motorway" ||
            highway_type == "motorway_link";

        // traffic rules
        if ( highway_type == "motorway" || highway_type == "motorway_link" ||
             highway_type == "trunk" || highway_type == "primary" || highway_type == "primary_link" ) {
            traffic_rules_ft = Tempus::TrafficRuleCar + Tempus::TrafficRuleTaxi + Tempus::TrafficRuleCarPool + Tempus::TrafficRuleTruck;
        }
        else if ( highway_type == "cycleway" ) {
            traffic_rules_ft = Tempus::TrafficRuleBicycle;
        }
        else if ( highway_type == "footway" ) {
            traffic_rules_ft = Tempus::TrafficRulePedestrian;
        }
        else {
            traffic_rules_ft = Tempus::TrafficRuleCar + Tempus::TrafficRuleTaxi + Tempus::TrafficRuleCarPool +
                Tempus::TrafficRuleTruck + Tempus::TrafficRulePedestrian + Tempus::TrafficRuleBicycle;
        }

        if ( !oneway ) {
            traffic_rules_tf = traffic_rules_ft;
        }
        else {
            if ( highway_type == "motorway" || highway_type == "motorway_link" ||
                 highway_type == "trunk" || highway_type == "primary" ||
                 highway_type == "primary_link" || highway_type == "cycleway" )
                traffic_rules_tf = 0;
            else
                traffic_rules_tf = Tempus::TrafficRulePedestrian;
        }

        // max car speed
        double max_speed = 0.0;
        auto max_speed_it = max_speed_.find( highway_type );
        if ( max_speed_it != max_speed_.end() )
            max_speed = max_speed_it->second;

        // road name
        std::string road_name;
        auto road_name_t = tags.find( "name" );
        if ( road_name_t != tags.end() )
            road_name = road_name_t->second;
        
        return { way_id, linestring_length( points ), traffic_rules_ft, traffic_rules_tf, max_speed, road_name };
    }
private:
    std::vector<Column> columns_;
    std::map<std::string, double> max_speed_;
};

DECLARE_DATA_PROFILE(tempus, TempusDataProfile);
