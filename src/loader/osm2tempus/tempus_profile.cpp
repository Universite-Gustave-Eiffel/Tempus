/**
 *   Copyright (C) 2016 Oslandia <infos@oslandia.com>
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

#include "data_profile.h"

#include "transport_modes.hh"
#include "geom.h"

#include <algorithm>

class TempusDataProfile : public DataProfile
{
public:
    TempusDataProfile() :
        DataProfile(),
        columns_( {
                { "vendor_id", DataType::UInt64Type },
                { "length", DataType::Float8Type },
                { "traffic_rules_ft", DataType::Int16Type },
                { "traffic_rules_tf", DataType::Int16Type },
                { "car_speed_limit", DataType::Float8Type },
                { "road_name", DataType::String }
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
        if ( highway_type == "motorway" || highway_type == "motorway_link" || highway_type == "trunk" ) {
            traffic_rules_ft = Tempus::TrafficRuleCar + Tempus::TrafficRuleTaxi + Tempus::TrafficRuleCarPool + Tempus::TrafficRuleTruck;
        }
        else if ( highway_type == "primary" || highway_type == "primary_link" ) {
            traffic_rules_ft = Tempus::TrafficRuleCar + Tempus::TrafficRuleTaxi + Tempus::TrafficRuleCarPool + Tempus::TrafficRuleTruck + Tempus::TrafficRuleBicycle + Tempus::TrafficRulePedestrian;
        }
        else if ( highway_type == "cycleway" ) {
            traffic_rules_ft = Tempus::TrafficRuleBicycle;
        }
        else if ( highway_type == "footway" || highway_type == "pedestrian" || highway_type == "steps" ) {
            traffic_rules_ft = Tempus::TrafficRulePedestrian;
        }
        else if ( highway_type == "path" || highway_type == "track" ) {
            traffic_rules_ft = Tempus::TrafficRulePedestrian + Tempus::TrafficRuleBicycle;
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
                 highway_type == "trunk" || highway_type == "cycleway" )
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
