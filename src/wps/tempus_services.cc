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

#include <iostream>
#include <iomanip>
#include <boost/format.hpp>

#include "wps_service.hh"
#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"
#include "db.hh"
#include "utils/graph_db_link.hh"
#include <boost/timer/timer.hpp>

using namespace Tempus;

//
// This file contains implementations of services offered by the Tempus WPS server.
// Variables are their XML schema are defined inside constructors.
// And the execute() method read from the XML tree or format the resulting XML tree.
// Pretty boring ...
//

namespace WPS {
void ensure_minimum_state( int state )
{
    int a_state = Application::instance()->state();

    if ( a_state < state ) {
        std::string state_str = to_string( a_state );
        std::cerr << "instance adress " << Application::instance() <<"\n";
        throw std::invalid_argument( "Invalid application state: " + state_str );
    }
}

///
/// "plugin_list" service, lists loaded plugins.
///
/// Output var: plugins, list of plugin names
///
class PluginListService : public Service {
public:
    PluginListService() : Service( "plugin_list" ) {
        add_output_parameter( "plugins" );
    };
    Service::ParameterMap execute( const ParameterMap& /*input_parameter_map*/ ) const {
        ParameterMap output_parameters;

        xmlNode* root_node = XML::new_node( "plugins" );
        std::vector<std::string> names( PluginFactory::instance()->plugin_list() );

        for ( size_t i=0; i<names.size(); i++ ) {
            xmlNode* node = XML::new_node( "plugin" );
            XML::new_prop( node,
                           "name",
                           names[i] );

            const Plugin::OptionDescriptionList& options = PluginFactory::instance()->option_descriptions( names[i] );
            Plugin::OptionDescriptionList::const_iterator it;

            for ( it = options.begin(); it != options.end(); it++ ) {
                xmlNode* option_node = XML::new_node( "option" );
                XML::new_prop( option_node, "name", it->first );
                XML::new_prop( option_node, "description", it->second.description );
                xmlNode* default_value_node = XML::new_node( "default_value" );
                xmlNode* value_node = 0;

                switch ( it->second.type() ) {
                case BoolVariant:
                    value_node = XML::new_node( "bool_value" );
                    break;
                case IntVariant:
                    value_node = XML::new_node( "int_value" );
                    break;
                case FloatVariant:
                    value_node = XML::new_node( "float_value" );
                    break;
                case StringVariant:
                    value_node = XML::new_node( "string_value" );
                    break;
                default:
                    throw std::invalid_argument( "Plugin " + names[i] + ": unknown type for option " + it->first );
                }

                XML::new_prop( value_node, "value", it->second.default_value.str() );
                XML::add_child( default_value_node, value_node );
                XML::add_child( option_node, default_value_node );

                XML::add_child( node, option_node );
            }

            const Plugin::PluginCapabilities& params = PluginFactory::instance()->plugin_capabilities( names[i] );
            for ( std::vector<CostId>::const_iterator cit = params.optimization_criteria().begin();
                  cit != params.optimization_criteria().end();
                  cit ++ ) {
                xmlNode* param_node = XML::new_node( "supported_criterion" );
                XML::add_child( param_node, XML::new_text( boost::lexical_cast<std::string>(static_cast<int>(*cit)) ) );

                XML::add_child( node, param_node );
            }
            {
                xmlNode *support_node = XML::new_node("intermediate_steps");
                XML::add_child( support_node, XML::new_text( params.intermediate_steps() ? "true" : "false" ) );
                XML::add_child( node, support_node );
            }
            {
                xmlNode *support_node = XML::new_node("depart_after");
                XML::add_child( support_node, XML::new_text( params.depart_after() ? "true" : "false" ) );
                XML::add_child( node, support_node );
            }
            {
                xmlNode *support_node = XML::new_node("arrive_before");
                XML::add_child( support_node, XML::new_text( params.arrive_before() ? "true" : "false" ) );
                XML::add_child( node, support_node );
            }

            XML::add_child( root_node, node );
        }

        output_parameters[ "plugins" ] = root_node;
        return output_parameters;
    };
};

///
/// "constant_list" service, outputs list of constants contained in the database (road type, transport type, transport networks).
///
/// Output var: transport_modes.
/// Output var: transport_networks.
///
class ConstantListService : public Service {
public:
    ConstantListService() : Service( "constant_list" ) {
        add_output_parameter( "transport_modes" );
        add_output_parameter( "transport_networks" );
    };
    Service::ParameterMap execute( const ParameterMap& /*input_parameter_map*/ ) const {
        ParameterMap output_parameters;

        const Tempus::Multimodal::Graph& graph = *Application::instance()->graph();

        {
            xmlNode* root_node = XML::new_node( "transport_modes" );
            Tempus::Multimodal::Graph::TransportModes::const_iterator it;

            for ( it = graph.transport_modes().begin(); it != graph.transport_modes().end(); it++ ) {
                xmlNode* node = XML::new_node( "transport_mode" );
                XML::new_prop( node, "id", it->first );
                XML::new_prop( node, "name", it->second.name() );
                XML::new_prop( node, "is_public_transport", it->second.is_public_transport() );
                XML::new_prop( node, "need_parking", it->second.need_parking() );
                XML::new_prop( node, "is_shared", it->second.is_shared() );
                XML::new_prop( node, "must_be_returned", it->second.must_be_returned() );
                XML::new_prop( node, "traffic_rules", it->second.traffic_rules() );
                XML::new_prop( node, "speed_rule", it->second.speed_rule() );
                XML::new_prop( node, "toll_rules", it->second.toll_rules() );
                XML::new_prop( node, "engine_type", it->second.engine_type() );
                XML::add_child( root_node, node );
            }

            output_parameters[ "transport_modes" ] = root_node;
        }

        {
            xmlNode* root_node = XML::new_node( "transport_networks" );
            Multimodal::Graph::NetworkMap::const_iterator it;

            for ( it = graph.network_map().begin(); it != graph.network_map().end(); it++ ) {
                xmlNode* node = XML::new_node( "transport_network" );
                XML::new_prop( node, "id", it->first );
                XML::new_prop( node, "name", it->second.name() );
                XML::add_child( root_node, node );
            }

            output_parameters[ "transport_networks" ] = root_node;
        }

        return output_parameters;
    };
};

Tempus::db_id_t road_vertex_id_from_coordinates( Db::Connection& db, double x, double y )
{
    //
    // Call to the stored procedure
    //
    std::string q = ( boost::format( "SELECT tempus.road_node_id_from_coordinates(%.3f, %.3f)" ) % x % y ).str();
    Db::Result res = db.exec( q );

    if ( (res.size() == 0) || (res[0][0].is_null()) ) {
        return 0;
    }

    return res[0][0].as<Tempus::db_id_t>();
}

Tempus::db_id_t road_vertex_id_from_coordinates_and_modes( Db::Connection& db, double x, double y, const std::vector<db_id_t>& modes )
{
    std::string array_modes = "array[";
    for ( size_t i = 0; i < modes.size(); i++ ) {
        array_modes += (boost::format("%d") % modes[i]).str();
        if (i<modes.size()-1) {
            array_modes += ",";
        }
    }
    array_modes += "]";
    std::string q = ( boost::format( "SELECT tempus.road_node_id_from_coordinates_and_modes(%.3f, %.3f, %s)" ) % x % y % array_modes ).str();
    Db::Result res = db.exec( q );

    if ( (res.size() == 0) || (res[0][0].is_null()) ) {
        return 0;
    }

    return res[0][0].as<Tempus::db_id_t>();
}

///
/// "result" service, get results from a path query.
///
/// Output var: results, see roadmap.hh
///
class SelectService : public Service {
public:
    SelectService() : Service( "select" ) {
        add_input_parameter( "plugin" );
        add_input_parameter( "request" );
        add_input_parameter( "options" );
        add_output_parameter( "results" );
        add_output_parameter( "metrics" );
    }

    void parse_constraint( const xmlNode* node, Request::TimeConstraint& constraint ) const {
        constraint.type( lexical_cast<int>( XML::get_prop( node, "type" ) ) );

        std::string date_time = XML::get_prop( node, "date_time" );
        const char* date_time_str = date_time.c_str();
        int day, month, year, hour, min;
        sscanf( date_time_str, "%04d-%02d-%02dT%02d:%02d", &year, &month, &day, &hour, &min );
        constraint.set_date_time( boost::posix_time::ptime( boost::gregorian::date( year, month, day ),
                                                        boost::posix_time::hours( hour ) + boost::posix_time::minutes( min ) ) );
    }

    Road::Vertex get_road_vertex_from_point( const xmlNode* node, Db::Connection& db ) const {
        Road::Vertex vertex;
        Tempus::db_id_t id;
        double x, y;

        const Tempus::Road::Graph& road_graph = Application::instance()->graph()->road();

        bool has_vertex = XML::has_prop( node, "vertex" );
        bool has_x = XML::has_prop( node, "x" );
        bool has_y = XML::has_prop( node, "y" );

        if ( ! ( ( has_x & has_y ) ^ has_vertex ) ) {
            throw std::invalid_argument( "Node " + XML::to_string( node ) + " must have either x and y or vertex" );
        }

        // if "vertex" attribute is present, use it
        if ( has_vertex ) {
            std::string v_str = XML::get_prop( node, "vertex" );
            id = lexical_cast<Tempus::db_id_t>( v_str );
        }
        else {
            // should have "x" and "y" attributes
            std::string x_str = XML::get_prop( node, "x" );
            std::string y_str = XML::get_prop( node, "y" );

            x = lexical_cast<double>( x_str );
            y = lexical_cast<double>( y_str );

            id = road_vertex_id_from_coordinates( db, x, y );

            if ( id == 0 ) {
                throw std::invalid_argument( ( boost::format( "Cannot find vertex id for %.3f, %.3f" ) % x % y ).str() );
            }
        }

        bool found;
        boost::tie( vertex, found ) = vertex_from_id( id, road_graph );
        return vertex;
    }

    Road::Vertex get_road_vertex_from_point_and_modes( const xmlNode* node, Db::Connection& db, const std::vector<db_id_t> modes ) const {
        Road::Vertex vertex;
        Tempus::db_id_t id;
        double x, y;

        const Tempus::Road::Graph& road_graph = Application::instance()->graph()->road();

        bool has_vertex = XML::has_prop( node, "vertex" );
        bool has_x = XML::has_prop( node, "x" );
        bool has_y = XML::has_prop( node, "y" );

        if ( ( has_x && has_y ) == has_vertex ) {
            throw std::invalid_argument( "Node " + XML::to_string( node ) + " must have either x and y or vertex" );
        }

        // if "vertex" attribute is present, use it
        if ( has_vertex ) {
            std::string v_str = XML::get_prop( node, "vertex" );
            id = lexical_cast<Tempus::db_id_t>( v_str );
        }
        else {
            // should have "x" and "y" attributes
            std::string x_str = XML::get_prop( node, "x" );
            std::string y_str = XML::get_prop( node, "y" );

            x = lexical_cast<double>( x_str );
            y = lexical_cast<double>( y_str );

            id = road_vertex_id_from_coordinates_and_modes( db, x, y, modes );

            if ( id == 0 ) {
                throw std::invalid_argument( ( boost::format( "Cannot find vertex id for %.3f, %.3f" ) % x % y ).str() );
            }
        }

        bool found;
        boost::tie( vertex, found ) = vertex_from_id( id, road_graph );
        return vertex;
    }

    Road::Vertex get_road_vertex_from_point_and_mode( const xmlNode* node, Db::Connection& db, db_id_t mode ) const {
        std::vector<db_id_t> modes;
        modes.push_back( mode );
        return get_road_vertex_from_point_and_modes( node, db, modes );
    }

    Service::ParameterMap execute( const ParameterMap& input_parameter_map ) const {
        ParameterMap output_parameters;
        /*
        */
#ifdef TIMING_ENABLED
#   define TIMING \
       std::cout << __LINE__ << " " <<(timer.elapsed().wall-start)*1.e-9 << " sec\n";\
       start = timer.elapsed().wall;
        boost::timer::cpu_timer timer;
        boost::timer::nanosecond_type start = timer.elapsed().wall;
        std::cout << std::fixed << std::setprecision( 5 );
#else
#   define TIMING
#endif
        // Ensure XML is OK
        Service::check_parameters( input_parameter_map, input_parameter_schema_ );
        ensure_minimum_state( Application::GraphBuilt );
        const xmlNode* plugin_node = input_parameter_map.find( "plugin" )->second;
        const std::string plugin_str = XML::get_prop( plugin_node, "name" );
        TIMING
        std::auto_ptr<Plugin> plugin( PluginFactory::instance()->createPlugin( plugin_str ) );
        TIMING

        Tempus::Request request;

        // get options
        {
            const xmlNode* options_node = input_parameter_map.find( "options" )->second;
            const xmlNode* field = XML::get_next_nontext( options_node->children );

            while ( field && !xmlStrcmp( field->name, ( const xmlChar* )"option" ) ) {
                std::string name = XML::get_prop( field, "name" );

                const xmlNode* value_node = XML::get_next_nontext( field->children );
                Tempus::VariantType t = Tempus::IntVariant;

                if ( !xmlStrcmp( value_node->name, ( const xmlChar* )"bool_value" ) ) {
                    t = Tempus::BoolVariant;
                }
                else if ( !xmlStrcmp( value_node->name, ( const xmlChar* )"int_value" ) ) {
                    t = Tempus::IntVariant;
                }
                else if ( !xmlStrcmp( value_node->name, ( const xmlChar* )"float_value" ) ) {
                    t = Tempus::FloatVariant;
                }
                else if ( !xmlStrcmp( value_node->name, ( const xmlChar* )"string_value" ) ) {
                    t = Tempus::StringVariant;
                }

                const std::string value = XML::get_prop( value_node, "value" );

                plugin->set_option_from_string( name, value, t );

                field = XML::get_next_nontext( field->next );
            }
        }
        TIMING

        // pre_process
        {
            Db::Connection db( Application::instance()->db_options() );

            // now extract actual data
            xmlNode* request_node = input_parameter_map.find( "request" )->second;

            const xmlNode* field = XML::get_next_nontext( request_node->children );

            // first, parse allowed modes
            {
                const xmlNode* sfield = field;
                // skip until the first allowed mode
                while ( sfield && xmlStrcmp( sfield->name, (const xmlChar*)"allowed_mode") ) {
                    sfield = XML::get_next_nontext( sfield->next );                    
                }
                // loop over allowed modes
                while ( sfield && !xmlStrcmp( sfield->name, ( const xmlChar* )"allowed_mode" ) ) {
                    db_id_t mode = lexical_cast<db_id_t>( sfield->children->content );
                    request.add_allowed_mode( mode );
                    sfield = XML::get_next_nontext( sfield->next );
                }
            }

            bool has_walking = std::find( request.allowed_modes().begin(), request.allowed_modes().end(), TransportModeWalking ) != request.allowed_modes().end();
            bool has_private_bike = std::find( request.allowed_modes().begin(), request.allowed_modes().end(), TransportModePrivateBicycle ) != request.allowed_modes().end();
            bool has_private_car = std::find( request.allowed_modes().begin(), request.allowed_modes().end(), TransportModePrivateCar ) != request.allowed_modes().end();

            // add walking if private car && private bike
            if ( !has_walking && has_private_car && has_private_bike ) {
                request.add_allowed_mode( TransportModeWalking );
            }
            // add walking if no other private mode is present
            if ( !has_walking && !has_private_car && !has_private_bike ) {
                request.add_allowed_mode( TransportModeWalking );
            }

            Request::Step origin;
            origin.set_location( get_road_vertex_from_point_and_mode( field, db, request.allowed_modes()[0] ) );
            
            request.set_origin( origin );

            // parking location id, optional
            const xmlNode* n = XML::get_next_nontext( field->next );

            if ( !xmlStrcmp( n->name, ( const xmlChar* )"parking_location" ) ) {
                request.set_parking_location( get_road_vertex_from_point( n, db ) );
                field = n;
            }

            // optimizing criteria
            field = XML::get_next_nontext( field->next );
            request.set_optimizing_criterion( 0, lexical_cast<int>( field->children->content ) );
            field = XML::get_next_nontext( field->next );

            while ( !xmlStrcmp( field->name, ( const xmlChar* )"optimizing_criterion" ) ) {
                request.add_criterion( static_cast<CostId>(lexical_cast<int>( field->children->content )) );
                field = XML::get_next_nontext( field->next );
            }

            // steps, 1 .. N
            while ( field && !xmlStrcmp( field->name, (const xmlChar*)"step" ) ) {
                Request::Step step;
                const xmlNode* subfield;
                // destination id
                subfield = XML::get_next_nontext( field->children );
                step.set_location( get_road_vertex_from_point_and_mode( subfield, db, request.allowed_modes()[0] ) );

                // constraint
                subfield = XML::get_next_nontext( subfield->next );
                Request::TimeConstraint constraint;
                parse_constraint( subfield, constraint );
                step.set_constraint( constraint );

                // private_vehicule_at_destination
                std::string val = XML::get_prop( field, "private_vehicule_at_destination" );
                step.set_private_vehicule_at_destination( val == "true" );

                // next step
                field = XML::get_next_nontext( field->next );
                if ( field && !xmlStrcmp( field->name, (const xmlChar*)"step" ) ) {
                    request.add_intermediary_step( step );
                }
                else {
                    // destination
                    request.set_destination( step );
                }
            }

            // call cycle
            plugin->cycle();
            TIMING

            // then call pre_process
            plugin->pre_process( request );
        }

        // process
        plugin->process();
        TIMING

        // metrics
        xmlNode* metrics_node = XML::new_node( "metrics" );
        Plugin::MetricValueList metrics = plugin->metrics();
        Plugin::MetricValueList::iterator it;

        for ( it = metrics.begin(); it != metrics.end(); it++ ) {
            xmlNode* metric_node = XML::new_node( "metric" );
            XML::new_prop( metric_node, "name", it->first );
            XML::new_prop( metric_node, "value",
                           plugin->metric_to_string( it->first ) );

            XML::add_child( metrics_node, metric_node );
        }

        output_parameters[ "metrics" ] = metrics_node;
        TIMING

        // result
        Tempus::Result& result = plugin->result();

        const Multimodal::Graph& graph_ = *Application::instance()->graph();

        xmlNode* root_node = XML::new_node( "results" );

        if ( result.size() == 0 ) {
            output_parameters["results"] = root_node;
            return output_parameters;
        }

        TIMING

        Tempus::Result::const_iterator rit;

        for ( rit = result.begin(); rit != result.end(); ++rit ) {
            const Tempus::Roadmap& roadmap = *rit;

            xmlNode* result_node = XML::new_node( "result" );

            for ( Roadmap::StepConstIterator sit = roadmap.begin(); sit != roadmap.end(); sit++ ) {
                xmlNode* step_node = 0;
                const Roadmap::Step* gstep = &*sit;

                if ( sit->step_type() == Roadmap::Step::RoadStep ) {
                    const Roadmap::RoadStep* step = static_cast<const Roadmap::RoadStep*>( &*sit );
                    step_node = XML::new_node( "road_step" );

                    XML::set_prop( step_node, "road", graph_.road()[step->road_edge()].road_name() );
                    XML::set_prop( step_node, "end_movement", to_string( step->end_movement() ) );
                }
                else if ( sit->step_type() == Roadmap::Step::PublicTransportStep ) {
                    const Roadmap::PublicTransportStep* step = static_cast<const Roadmap::PublicTransportStep*>( &*sit );

                    if ( graph_.public_transports().find( step->network_id() ) == graph_.public_transports().end() ) {
                        throw std::runtime_error( ( boost::format( "Can't find PT network ID %1%" ) % step->network_id() ).str() );
                    }

                    const PublicTransport::Graph& pt_graph = *graph_.public_transport( step->network_id() );

                    step_node = XML::new_node( "public_transport_step" );

                    XML::set_prop( step_node, "network", graph_.network( step->network_id() )->name() );

                    std::string departure_str;
                    PublicTransport::Vertex v1 = step->departure_stop();
                    PublicTransport::Vertex v2 = step->arrival_stop();

                    departure_str = pt_graph[ v1 ].name();
                    std::string arrival_str = pt_graph[ v2 ].name();
                    XML::set_prop( step_node, "departure_stop", departure_str );
                    XML::set_prop( step_node, "arrival_stop", arrival_str );
                    XML::set_prop( step_node, "route", step->route() );
                    XML::set_prop( step_node, "trip_id", to_string(step->trip_id()) );
                    XML::set_prop( step_node, "departure_time", to_string(step->departure_time()) );
                    XML::set_prop( step_node, "arrival_time", to_string(step->arrival_time()) );
                    XML::set_prop( step_node, "wait_time", to_string(step->wait()) );
                }
                else if ( sit->step_type() == Roadmap::Step::TransferStep ) {
                    const Roadmap::TransferStep* step = static_cast<const Roadmap::TransferStep*>( &*sit );
                    const Multimodal::Edge* edge = static_cast<const Multimodal::Edge*>( step );

                    std::string road_name, stop_name;
                    std::string type_str = to_string( edge->connection_type() );

                    const PublicTransport::Graph* pt_graph = 0;
                    bool road_transport = false;
                    road_name = graph_.road()[edge->road_edge()].road_name();
                    if ( edge->connection_type() == Multimodal::Edge::Road2Transport ) {
                        pt_graph = edge->target().pt_graph();
                        stop_name = ( *pt_graph )[edge->target().pt_vertex()].name();
                        road_transport = true;
                    }
                    else if ( edge->connection_type() == Multimodal::Edge::Transport2Road ) {
                        pt_graph = edge->source().pt_graph();
                        stop_name = ( *pt_graph )[edge->source().pt_vertex()].name();
                        road_transport = true;
                    }

                    if ( road_transport ) {
                        db_id_t network_id = 0;

                        for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports().begin();
                              nit != graph_.public_transports().end();
                              ++nit ) {
                            if ( nit->second == pt_graph ) {
                                network_id = nit->first;
                                break;
                            }
                        }

                        step_node = XML::new_node( "road_transport_step" );

                        XML::set_prop( step_node, "type", type_str );
                        XML::set_prop( step_node, "road", road_name );
                        XML::set_prop( step_node, "network", graph_.network( network_id )->name() );
                        XML::set_prop( step_node, "stop", stop_name );
                    }
                    else {
                        std::string poi_name = "";
                        step_node = XML::new_node( "transfer_step" );
                        if ( edge->connection_type() == Multimodal::Edge::Road2Poi ) {
                            poi_name = edge->target().poi()->name();
                        }
                        else if ( edge->connection_type() == Multimodal::Edge::Poi2Road ) {
                            poi_name = edge->source().poi()->name();
                        }
                        XML::set_prop( step_node, "type", type_str );
                        XML::set_prop( step_node, "road", road_name );
                        XML::set_prop( step_node, "poi", poi_name );
                        XML::set_prop( step_node, "final_mode", to_string( step->final_mode() ) );
                    }
                }
                BOOST_ASSERT( step_node );

                // transport_mode
                XML::new_prop( step_node, "transport_mode", to_string(sit->transport_mode()) );

                for ( Tempus::Costs::const_iterator cit = gstep->costs().begin(); cit != gstep->costs().end(); cit++ ) {
                    xmlNode* cost_node = XML::new_node( "cost" );
                    XML::new_prop( cost_node,
                                   "type",
                                   to_string( cit->first ) );
                    XML::new_prop( cost_node,
                                   "value",
                                   to_string( cit->second ) );
                    XML::add_child( step_node, cost_node );
                }

                XML::set_prop( step_node, "wkb", sit->geometry_wkb() );

                XML::add_child( result_node, step_node );
            }

            TIMING

            // total costs

                Costs total_costs( get_total_costs(roadmap) );
                for ( Tempus::Costs::const_iterator cit = total_costs.begin(); cit != total_costs.end(); cit++ ) {
                    xmlNode* cost_node = XML::new_node( "cost" );
                    XML::new_prop( cost_node,
                                   "type",
                                   to_string( cit->first ) );
                    XML::new_prop( cost_node,
                                   "value",
                                   to_string( cit->second ) );
                    XML::add_child( result_node, cost_node );
                }

                {
                    xmlNode* starting_dt_node = XML::new_node( "starting_date_time" );
                    std::string dt_string = boost::posix_time::to_iso_extended_string( roadmap.starting_date_time() );
                    XML::add_child( starting_dt_node, XML::new_text( dt_string ) );
                    XML::add_child( result_node, starting_dt_node );
                }

                // path trace
                if ( roadmap.trace().size() ) {
                    xmlNode * trace_node = XML::new_node( "trace" );

                    for ( size_t i = 0; i < roadmap.trace().size(); i++ ) {
                        xmlNode *edge_node = XML::new_node("edge");
                        const ValuedEdge& ve = roadmap.trace()[i];

                        XML::set_prop( edge_node, "wkb", ve.geometry_wkb() );

                        Multimodal::Vertex orig = ve.source();
                        Multimodal::Vertex dest = ve.target();

                        xmlNode *orig_node = 0;
                        if ( orig.type() == Multimodal::Vertex::Road ) {
                            orig_node = XML::new_node("road");
                            XML::set_prop(orig_node, "id", to_string((*orig.road_graph())[orig.road_vertex()].db_id()) );
                        }
                        else if ( orig.type() == Multimodal::Vertex::PublicTransport ) {
                            orig_node = XML::new_node("pt");
                            XML::set_prop(orig_node, "id", to_string((*orig.pt_graph())[orig.pt_vertex()].db_id()) );
                        }
                        else if ( orig.type() == Multimodal::Vertex::Poi ) {
                            orig_node = XML::new_node("poi");
                            XML::set_prop(orig_node, "id", to_string(orig.poi()->db_id()));
                        }
                        if (orig_node) {
                            XML::add_child(edge_node, orig_node);
                        }

                        xmlNode *dest_node = 0;
                        if ( dest.type() == Multimodal::Vertex::Road ) {
                            dest_node = XML::new_node("road");
                            XML::set_prop(dest_node, "id", to_string((*dest.road_graph())[dest.road_vertex()].db_id()) );
                        }
                        else if ( dest.type() == Multimodal::Vertex::PublicTransport ) {
                            dest_node = XML::new_node("pt");
                            XML::set_prop(dest_node, "id", to_string((*dest.pt_graph())[dest.pt_vertex()].db_id()) );
                        }
                        else if ( dest.type() == Multimodal::Vertex::Poi ) {
                            dest_node = XML::new_node("poi");
                            XML::set_prop(dest_node, "id", to_string(dest.poi()->db_id()));
                        }
                        if (dest_node) {
                            XML::add_child(edge_node, dest_node);
                        }

                        VariantMap::const_iterator vit;
                        for ( vit = ve.values().begin(); vit != ve.values().end(); ++vit ) {
                            xmlNode *n = 0;
                            if (vit->second.type() == BoolVariant) {
                                n = XML::new_node("b");
                            }
                            else if (vit->second.type() == IntVariant) {
                                n = XML::new_node("i");
                            }
                            else if (vit->second.type() == FloatVariant) {
                                n = XML::new_node("f");
                            }
                            else if (vit->second.type() == StringVariant) {
                                n = XML::new_node("s");
                            }
                            if (n ) {
                                XML::set_prop(n, "k", vit->first);
                                XML::set_prop(n, "v", vit->second.str());
                                XML::add_child(edge_node, n);
                            }
                        }
                        XML::add_child(trace_node, edge_node);
                    }
                    XML::add_child(result_node, trace_node);
                }

            XML::add_child( root_node, result_node );
        } // for each result

        output_parameters[ "results" ] = root_node;

#ifdef TIMING_ENABLED
        timer.stop();
        std::cout << timer.elapsed().wall*1.e-9 << " in select " << plugin->name()<< "\n";
#endif
#undef TIMING
        return output_parameters;
    }
};

static PluginListService plugin_list_service;
static SelectService select_service;
static ConstantListService constant_list_service;

} // WPS namespace
