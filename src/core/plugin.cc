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

#include <string>
#include <iostream>
#include <memory>

#include <boost/format.hpp>

#include "plugin.hh"
#include "utils/graph_db_link.hh"
#ifdef _WIN32
#include <strsafe.h>
#endif

#include "plugin_factory.hh"

namespace Tempus
{

Plugin::Plugin( const std::string& nname, const VariantMap& options ) :
    name_( nname )
{
    schema_name_ = get_option_or_default( options, "db/schema" ).str();
    db_options_ = get_option_or_default( options, "db/options" ).str();
}

Plugin::OptionDescriptionList Plugin::common_option_descriptions()
{
    Plugin::OptionDescriptionList opt;
    declare_option(opt, "db/options", "DB connection options", Variant::from_string(""));
    declare_option(opt, "db/schema", "DB schema name", Variant::from_string("tempus"));
    return opt;
}

///
/// Process the user request.
std::unique_ptr<Result> PluginRequest::process( const Request& /*request*/ )
{
    COUT << "[plugin_base]: process" << std::endl;
    return std::unique_ptr<Result>( new Result );
}

PluginRequest::PluginRequest( const Plugin* plugin, const VariantMap& options ) :
    plugin_(plugin), options_( options )
{
    // default metrics
    metrics_[ "time_s" ] = Variant::from_float(0.0);
    metrics_[ "iterations" ] = Variant::from_int(0);
}

Plugin::OptionDescriptionList option_descriptions( const Plugin* plugin )
{
    return PluginFactory::instance()->option_descriptions( plugin->name() );
}

Variant Plugin::get_option_or_default( const VariantMap& options, const std::string& key ) const
{
    if ( options.find( key ) != options.end() ) {
        return options.find( key )->second;
    }
    // look for the default value
    Plugin::OptionDescriptionList desc = option_descriptions( this );
    auto it = desc.find( key );
    if ( it != desc.end() ) {
        return it->second.default_value;
    }
    return Variant();
}

template <class T>
void PluginRequest::get_option( const std::string& nname, T& value ) const
{
    const Plugin::OptionDescriptionList desc = option_descriptions( plugin_ );
    auto descIt = desc.find( nname );

    if ( descIt == desc.end() ) {
        throw std::runtime_error( "get_option(): cannot find option " + nname );
    }

    auto it = options_.find( nname );
    if ( it == options_.end() ) {
        // return default value
        value = descIt->second.default_value.as<T>();
        return;
    }

    value = it->second.as<T>();
}
// template instanciations
template void PluginRequest::get_option<bool>( const std::string&, bool& ) const;
template void PluginRequest::get_option<int64_t>( const std::string&, int64_t& ) const;
template void PluginRequest::get_option<double>( const std::string&, double& ) const;
template void PluginRequest::get_option<std::string>( const std::string&, std::string& ) const;

bool PluginRequest::get_bool_option( const std::string& name ) const
{
    bool v;
    get_option( name, v );
    return v;
}

int64_t PluginRequest::get_int_option( const std::string& name ) const
{
    int64_t v;
    get_option( name, v );
    return v;
}

double PluginRequest::get_float_option( const std::string& name ) const
{
    double v;
    get_option( name, v );
    return v;
}

std::string PluginRequest::get_string_option( const std::string& name ) const
{
    std::string v;
    get_option( name, v );
    return v;
}

std::string PluginRequest::metric_to_string( const std::string& nname ) const
{
    if ( metrics_.find( nname ) == metrics_.end() ) {
        throw std::invalid_argument( "Cannot find metric " + nname );
    }

    const MetricValue& v = metrics_.find( nname )->second;
    return v.str();
}

///
/// Text formatting and preparation of roadmap
void simple_multimodal_roadmap( Result& result, Db::Connection& db, const Multimodal::Graph& graph )
{
    for ( Result::iterator rit = result.begin(); rit != result.end(); ++rit ) {
        Roadmap& roadmap = rit->roadmap();
        Roadmap new_roadmap;
        const Road::Graph& road_graph = graph.road();

        Tempus::db_id_t previous_section = 0;
        bool on_roundabout = false;
        bool was_on_roundabout = false;
        Roadmap::RoadStep* last_step = 0;

        Roadmap::RoadStep::EndMovement movement;

        // public transport step accumulator
        std::vector< std::pair< db_id_t, db_id_t > > accum_pt;
        Roadmap::PublicTransportStep* pt_first = 0;

        // road step accumulator
        std::vector< db_id_t > accum_road;

        // cost accumulator
        Costs accum_costs;

        // retrieve data from db
        fill_roadmap_from_db( roadmap.begin(), roadmap.end(), db );

        Roadmap::StepIterator it = roadmap.begin();
        Roadmap::StepIterator next = it;
        if ( it == roadmap.end() ) {
            continue;
        }
        next++;

        for ( ; it != roadmap.end(); it++ ) {
            if ( it->step_type() == Roadmap::Step::TransferStep ) {
                Roadmap::TransferStep* step = static_cast<Roadmap::TransferStep*>( &*it );
                new_roadmap.add_step( std::auto_ptr<Roadmap::Step>(step->clone()) );
            }
            else if ( it->step_type() == Roadmap::Step::PublicTransportStep ) {
                Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( &*it );

                // store stops
                accum_pt.push_back( std::make_pair(step->departure_stop(), step->arrival_stop()) );
                // accumulate costs
                for ( Costs::const_iterator cit = step->costs().begin(); cit != step->costs().end(); ++cit ) {
                    accum_costs[cit->first] += cit->second;
                }
                if ( !pt_first ) {
                    pt_first = step;
                }

                // accumulate steps sharing the same trip
                if ( next != roadmap.end() && next->step_type() == Roadmap::Step::PublicTransportStep ) {
                    Roadmap::PublicTransportStep* next_step = static_cast<Roadmap::PublicTransportStep*>( &*next );
                    if ( next_step->trip_id() == step->trip_id() ) {
                        if ( next != roadmap.end() ) next++;
                        continue;
                    }
                }

                // retrieval of the step's geometry
                // TODO: do not call postgis just to merge geometries
                std::string q = "SELECT st_asbinary(st_linemerge(st_collect(geom))) FROM (SELECT geom FROM tempus.pt_section WHERE ARRAY[stop_from,stop_to] IN (";
                for ( size_t i = 0; i < accum_pt.size(); i++ ) {
                    q += (boost::format("ARRAY[%1%,%2%]") % accum_pt[i].first % accum_pt[i].second ).str();
                    if ( i != accum_pt.size()-1 ) {
                        q += ",";
                    }
                }
                q += ")) t";
                Db::Result res = db.exec( q );
                if ( ! res.size() ) {
                    throw std::runtime_error("No result for " + q);
                }
                std::string wkb = res[0][0].as<std::string>();
                // get rid of the heading '\x'
                if ( wkb.size() > 2 )
                    wkb = wkb.substr( 2 );
                step->set_geometry_wkb( wkb );

                // copy info from the first step
                step->set_transport_mode( pt_first->transport_mode() );
                step->set_route( pt_first->route() );
                step->set_departure_stop( pt_first->departure_stop() );
                step->set_departure_time( pt_first->departure_time() );
                step->set_wait( pt_first->wait() );

                // set costs from the accumulator
                for ( Costs::const_iterator cit = accum_costs.begin(); cit != accum_costs.end(); ++cit ) {
                    step->set_cost( static_cast<CostId>(cit->first), cit->second );
                }
                accum_costs.clear();

                new_roadmap.add_step( std::auto_ptr<Roadmap::Step>(step->clone()) );

                accum_pt.clear();
                pt_first = 0;
            }
            else if ( it->step_type() == Roadmap::Step::RoadStep ) {

                Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( &*it );

                // accumulate step sharing the same road name
                accum_road.push_back( step->road_edge_id() );
                // accumulate costs
                for ( Costs::const_iterator cit = step->costs().begin(); cit != step->costs().end(); ++cit ) {
                    accum_costs[cit->first] += cit->second;
                }

                if ( next != roadmap.end() && next->step_type() == Roadmap::Step::RoadStep ) {
                    Roadmap::RoadStep* next_step = static_cast<Roadmap::RoadStep*>( &*next );
                    if ( next_step->road_name() == step->road_name() ) {
                        if ( next != roadmap.end() )
                            next++;
                        continue;
                    }
                }

                //
                // retrieval of the step's geometry
                {
                    // TODO: do not call postgis just to merge geometries
                    std::string q = "SELECT st_asbinary(st_linemerge(st_collect(geom))) FROM (SELECT geom FROM tempus.road_section WHERE id IN (";
                    for ( size_t i = 0; i < accum_road.size(); i++ ) {
                        q += (boost::format("%1%") % accum_road[i]).str();
                        if ( i != accum_road.size()-1 ) {
                            q += ",";
                        }
                    }
                    q += ")) t";
                    // reverse the geometry if needed
                    Db::Result res = db.exec( q );
                    if ( ! res.size() ) {
                        throw std::runtime_error("No result for " + q);
                    }
                    std::string wkb = res[0][0].as<std::string>();

                    // get rid of the heading '\x'
                    if ( wkb.size() > 0 ) {
                        step->set_geometry_wkb( wkb.substr( 2 ) );
                    }
                    else {
                        step->set_geometry_wkb( "" );
                    }
                }

                //
                // For a road step, we have to compute directions of turn
                //
                movement = Roadmap::RoadStep::GoAhead;

                on_roundabout = road_graph[graph.road_edge_from_id(step->road_edge_id()).get()].is_roundabout();

                bool action = false;

                if ( on_roundabout && !was_on_roundabout ) {
                    // we enter a roundabout
                    movement = Roadmap::RoadStep::RoundAboutEnter;
                    action = true;
                }

                if ( !on_roundabout && was_on_roundabout ) {
                    // we leave a roundabout
                    // FIXME : compute the exit number
                    movement = Roadmap::RoadStep::FirstExit;
                    action = true;
                }

                if ( previous_section && !on_roundabout && !action ) {
                    // TODO: do not call postgis just to compute an angle
                    std::string q1 = ( boost::format( "SELECT ST_Azimuth( st_endpoint(s1.geom), st_startpoint(s1.geom) ), ST_Azimuth( st_startpoint(s2.geom), st_endpoint(s2.geom) ), st_endpoint(s1.geom)=st_startpoint(s2.geom) "
                                                      "FROM tempus.road_section AS s1, tempus.road_section AS s2 WHERE s1.id=%1% AND s2.id=%2%" ) % previous_section % step->road_edge_id() ).str();
                    Db::Result res = db.exec( q1 );
                    if ( ! res.size() ) {
                        throw std::runtime_error("No result for " + q1);
                    }
                    double pi = 3.14159265;
                    double z1 = res[0][0].as<double>() / pi * 180.0;
                    double z2 = res[0][1].as<double>() / pi * 180.0;
                    bool continuous =  res[0][2].as<bool>();

                    if ( !continuous ) {
                        z1 = z1 - 180;
                    }

                    int z = int( z1 - z2 );
                    z = ( z + 360 ) % 360;

                    if ( z >= 30 && z <= 150 ) {
                        movement = Roadmap::RoadStep::TurnRight;
                    }

                    if ( z >= 210 && z < 330 ) {
                        movement = Roadmap::RoadStep::TurnLeft;
                    }
                }

#if 1
                if ( last_step ) {
                    last_step->set_end_movement( movement );
                    last_step->set_distance_km( -1.0 );
                }
#endif

                was_on_roundabout = on_roundabout;

                // set costs from the accumulator
                for ( Costs::const_iterator cit = accum_costs.begin(); cit != accum_costs.end(); ++cit ) {
                    step->set_cost( static_cast<CostId>(cit->first), cit->second );
                }
                accum_costs.clear();

                new_roadmap.add_step( std::auto_ptr<Roadmap::Step>(step->clone()) );

                if ( next != roadmap.end() && next->step_type() != Roadmap::Step::RoadStep ) {
                    previous_section = 0;
                    last_step = 0;
                }
                else {
                    previous_section = step->road_edge_id();
                    Roadmap::StepIterator ite = new_roadmap.end();
                    ite--;
                    last_step = static_cast<Roadmap::RoadStep*>(&*ite);
                }

                accum_road.clear();
            }
            if (next != roadmap.end()) next++;
        }
#if 1
        if ( last_step ) {
            last_step->set_end_movement( Roadmap::RoadStep::YouAreArrived );
            last_step->set_distance_km( -1.0 );
        }
#endif

        new_roadmap.set_starting_date_time( roadmap.starting_date_time() );

        // set trace wkb
        if ( roadmap.trace().size() ) {
            PathTrace new_trace;
            for ( size_t i = 0; i < roadmap.trace().size(); i++ ) {
                ValuedEdge ve = roadmap.trace()[i];
                std::string wkb, name1, name2;
                get_edge_info_from_db( ve, db, wkb, name1, name2 );
                ve.set_geometry_wkb( wkb );
                new_trace.push_back( ve );
            }
            new_roadmap.set_trace( new_trace );
        }

        *rit = new_roadmap;
	}
}

}
