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

namespace WPS
{
    void ensure_minimum_state( int state )
    {
	int a_state = Application::instance()->state();
	if ( a_state < state )
	{
	    std::string state_str = to_string(a_state);
            std::cerr << "instance adress " << Application::instance() <<"\n";
	    throw std::invalid_argument( "Invalid application state: " + state_str );
	}
    }
    
    ///
    /// "plugin_list" service, lists loaded plugins.
    ///
    /// Output var: plugins, list of plugin names
    ///
    class PluginListService : public Service
    {
    public:
	PluginListService() : Service("plugin_list")
	{
	    add_output_parameter( "plugins" );
	};
	Service::ParameterMap execute( const ParameterMap& /*input_parameter_map*/ ) const
	{
            ParameterMap output_parameters;
	    
	    xmlNode* root_node = XML::new_node( "plugins" );
	    std::vector<std::string> names( PluginFactory::instance()->plugin_list() );
	    for ( size_t i=0; i<names.size(); i++ )
	    {
		xmlNode* node = XML::new_node( "plugin" );
		XML::new_prop( node,
			       "name",
			       names[i] );

                Plugin::OptionDescriptionList options = PluginFactory::instance()->option_descriptions( names[i] );
                Plugin::OptionDescriptionList::iterator it;
                for ( it = options.begin(); it != options.end(); it++ )
                {
                    xmlNode* option_node = XML::new_node( "option" );
                    XML::new_prop( option_node, "name", it->first );
                    XML::new_prop( option_node, "type",
                                   to_string( it->second.type ) );
                    XML::new_prop( option_node, "description", it->second.description );
                    XML::new_prop( option_node, "default_value", Plugin::to_string( it->second.default_value ));
                    XML::add_child( node, option_node );
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
    /// Output var: road_types.
    /// Output var: transport_types.
    /// Output var: transport_networks.
    ///
    class ConstantListService : public Service
    {
    public:
	ConstantListService() : Service("constant_list")
	{
	    add_output_parameter( "road_types" );
	    add_output_parameter( "transport_types" );
	    add_output_parameter( "transport_networks" );
	};
	Service::ParameterMap execute( const ParameterMap& /*input_parameter_map*/ ) const
	{
            ParameterMap output_parameters;
	    
	    Tempus::Multimodal::Graph& graph = Application::instance()->graph();

	    {
		xmlNode* root_node = XML::new_node( "road_types" );
		Tempus::RoadTypes::iterator it;
		for ( it = graph.road_types.begin(); it != graph.road_types.end(); it++ )
		{
		    xmlNode* node = XML::new_node( "road_type" );
		    XML::new_prop( node, "id", it->first );
		    XML::new_prop( node, "name", it->second.name );
		    XML::add_child( root_node, node );
		}
		output_parameters[ "road_types" ] = root_node;
	    }
	    
	    {
		xmlNode* root_node = XML::new_node( "transport_types" );
		Tempus::TransportTypes::iterator it;
		for ( it = graph.transport_types.begin(); it != graph.transport_types.end(); it++ )
		{
		    xmlNode* node = XML::new_node( "transport_type" );
		    XML::new_prop( node, "id", it->first );
		    XML::new_prop( node, "parent_id", it->second.parent_id );
		    XML::new_prop( node, "name", it->second.name );
		    XML::new_prop( node, "need_parking", it->second.need_parking );
		    XML::new_prop( node, "need_station", it->second.need_station );
		    XML::new_prop( node, "need_return", it->second.need_return );
		    XML::new_prop( node, "need_network", it->second.need_network );
		    XML::add_child( root_node, node );
		}
		output_parameters[ "transport_types" ] = root_node;
	    }
	    
	    {
		xmlNode* root_node = XML::new_node( "transport_networks" );
		Multimodal::Graph::NetworkMap::iterator it;
		for ( it = graph.network_map.begin(); it != graph.network_map.end(); it++ )
		{
		    xmlNode* node = XML::new_node( "transport_network" );
		    XML::new_prop( node, "id", it->first );
		    XML::new_prop( node, "name", it->second.name );
		    XML::new_prop( node, "provided_transport_types", it->second.provided_transport_types );
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
	std::string q = (boost::format( "SELECT tempus.road_node_id_from_coordinates(%1%, %2%)") % x % y).str();
	Db::Result res = db.exec( q );
	if ( res.size() == 0 )
	    return 0;
	return res[0][0].as<Tempus::db_id_t>();
    }

    ///
    /// "result" service, get results from a path query.
    ///
    /// Output var: results, see roadmap.hh
    ///
    class SelectService : public Service
    {
    public:
	SelectService() : Service( "select" )
	{
	    add_input_parameter( "plugin" );
	    add_input_parameter( "request" );
	    add_input_parameter( "options" );
	    add_output_parameter( "results" );
	    add_output_parameter( "metrics" );
	}

	void parse_constraint( const xmlNode* node, Request::TimeConstraint& constraint ) const
	{
	    constraint.type = lexical_cast<int>( XML::get_prop( node, "type") );
	    
		std::string date_time = XML::get_prop( node, "date_time" );
	    const char* date_time_str = date_time.c_str();
	    int day, month, year, hour, min;
	    sscanf(date_time_str, "%04d-%02d-%02dT%02d:%02d", &year, &month, &day, &hour, &min );
	    constraint.date_time = boost::posix_time::ptime(boost::gregorian::date(year, month, day),
                                                            boost::posix_time::hours( hour ) + boost::posix_time::minutes( min ) );	    
	}
	
        Road::Vertex get_road_vertex_from_point( const xmlNode* node, Db::Connection& db ) const
        {
            Road::Vertex vertex;
            Tempus::db_id_t id;
            double x, y;

            Tempus::Road::Graph& road_graph = Application::instance()->graph().road;

            bool has_vertex = XML::has_prop( node, "vertex" );
            bool has_x = XML::has_prop( node, "x" );
            bool has_y = XML::has_prop( node, "y" );

            if ( ! ((has_x & has_y) ^ has_vertex) ) {
                throw std::invalid_argument( "Node " + XML::to_string(node) + " must have either x and y or vertex" );
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
                if ( id == 0 )
                {
                    throw std::invalid_argument( (boost::format("Cannot find vertex id from %1%, %2%") % x % y).str() );
                }
            }
            vertex = vertex_from_id( id, road_graph );
            return vertex;
    }
        
	Service::ParameterMap execute( const ParameterMap& input_parameter_map ) const
	{
            ParameterMap output_parameters;
/*
*/
#ifdef TIMING_ENABLED
#   define TIMING \
       std::cout << __LINE__ << " " <<(timer.elapsed().wall-start)*1.e-9 << " sec\n";\
       start = timer.elapsed().wall;
            boost::timer::cpu_timer timer;
            boost::timer::nanosecond_type start = timer.elapsed().wall;
            std::cout << std::fixed << std::setprecision(5);
#else
#   define TIMING 
#endif
            // Ensure XML is OK
            Service::check_parameters( input_parameter_map, input_parameter_schema_ );
            ensure_minimum_state( Application::GraphBuilt );
            const xmlNode* plugin_node = input_parameter_map.find("plugin")->second;
            const std::string plugin_str = XML::get_prop( plugin_node, "name" );
            TIMING
            std::auto_ptr<Plugin> plugin( PluginFactory::instance()->createPlugin( plugin_str ));
            TIMING

            Tempus::Request request;

            // get options
            {
                const xmlNode* options_node = input_parameter_map.find("options")->second;
                const xmlNode* field = XML::get_next_nontext( options_node->children );
                while ( field && !xmlStrcmp( field->name, (const xmlChar*)"option" ) )
                {
                    std::string name = XML::get_prop( field, "name" );
                    std::string value = XML::get_prop( field, "value" );

                    plugin->set_option_from_string( name, value );
                    field = XML::get_next_nontext( field->next );       
                }
            }
            TIMING

            // pre_process
            {
                Db::Connection db( Application::instance()->db_options() );
            
                // now extract actual data
                xmlNode* request_node = input_parameter_map.find("request")->second;
                // allowed transport types
                request.allowed_transport_types = lexical_cast<int>( XML::get_prop( request_node, "allowed_transport_types" ) );

                const xmlNode* field = XML::get_next_nontext( request_node->children );
            
                request.origin = get_road_vertex_from_point( field, db );
                    
                // departure_constraint
                field = XML::get_next_nontext( field->next );
                parse_constraint( field, request.departure_constraint );
            
                // parking location id, optional
                const xmlNode *n = XML::get_next_nontext( field->next );
                if ( !xmlStrcmp( n->name, (const xmlChar*)"parking_location" ) )
                {
                    request.parking_location = get_road_vertex_from_point( n, db );
                    field = n;
                }
            
                // optimizing criteria
                request.optimizing_criteria.clear();
                field = XML::get_next_nontext( field->next );
                request.optimizing_criteria.push_back( lexical_cast<int>( field->children->content ) );
                field = XML::get_next_nontext( field->next );   
                while ( !xmlStrcmp( field->name, (const xmlChar*)"optimizing_criterion" ) )
                {
                    request.optimizing_criteria.push_back( lexical_cast<int>( field->children->content ) );
                    field = XML::get_next_nontext( field->next );       
                }
            
                // allowed networks, 1 .. N
                request.allowed_networks.clear();
                while ( !xmlStrcmp( field->name, (const xmlChar *)"allowed_network" ) )
                {
                    Tempus::db_id_t network_id = lexical_cast<Tempus::db_id_t>(field->children->content);
                    request.allowed_networks.push_back( network_id );
                    field = XML::get_next_nontext( field->next );
                }
        
                // steps, 1 .. N
                request.steps.clear();
                while ( field )
                {
                    request.steps.resize( request.steps.size() + 1 );
            
                    const xmlNode *subfield;
                    // destination id
                    subfield = XML::get_next_nontext( field->children );
                    request.steps.back().destination = get_road_vertex_from_point( subfield, db );

                    // constraint
                    subfield = XML::get_next_nontext( subfield->next );
                    parse_constraint( subfield, request.steps.back().constraint );
            
                    // private_vehicule_at_destination
                    std::string val = XML::get_prop( field, "private_vehicule_at_destination" );
                    request.steps.back().private_vehicule_at_destination = ( val == "true" );
            
                    // next step
                    field = XML::get_next_nontext( field->next ); 
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
            xmlNode * metrics_node = XML::new_node( "metrics" );
            Plugin::MetricValueList metrics = plugin->metrics();
            Plugin::MetricValueList::iterator it;
            for ( it = metrics.begin(); it != metrics.end(); it++ )
            {
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
            
            Multimodal::Graph& graph_ = Application::instance()->graph();
            Tempus::Road::Graph& road_graph = graph_.road;
            
            xmlNode* root_node = XML::new_node( "results" );
            if ( result.size() == 0 )
            {
                output_parameters["results"] = root_node;
                return output_parameters;
            }
            TIMING

            Tempus::Result::const_iterator rit;
            for ( rit = result.begin(); rit != result.end(); ++rit ) {
                const Tempus::Roadmap& roadmap = *rit;
                
                xmlNode* result_node = XML::new_node( "result" );
                Roadmap::StepList::const_iterator sit;
                for ( sit = roadmap.steps.begin(); sit != roadmap.steps.end(); sit++ )
                {
                    xmlNode* step_node = 0;
                    Roadmap::Step* gstep = *sit;
                    
                    if ( (*sit)->step_type == Roadmap::Step::RoadStep )
                    {
                        Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *sit );
                        step_node = XML::new_node( "road_step" );

                        std::string road_name = "Unknown road";
                        road_name = road_graph[ step->road_section].road_name;
                        XML::set_prop( step_node, "road", road_name );
                        XML::set_prop( step_node, "end_movement", to_string( step->end_movement ) );
                    }
                    else if ( (*sit)->step_type == Roadmap::Step::PublicTransportStep )
                    {
                        Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( *sit );
                        
                        if ( graph_.public_transports.find( step->network_id ) == graph_.public_transports.end() )
                        {
                            // can't find the pt network
                        }
                        PublicTransport::Graph& pt_graph = graph_.public_transports[ step->network_id ];
                        
                        step_node = XML::new_node( "public_transport_step" );
                        
                        XML::set_prop( step_node, "network", graph_.network_map[ step->network_id ].name );
                        
                        std::string departure_str;
                        PublicTransport::Vertex v1 = vertex_from_id( pt_graph[step->section].stop_from, pt_graph );
                        PublicTransport::Vertex v2 = vertex_from_id( pt_graph[step->section].stop_to, pt_graph );
                        departure_str = pt_graph[ v1 ].name;
                        std::string arrival_str = pt_graph[ v2 ].name;
                        XML::set_prop( step_node, "departure_stop", departure_str );
                        XML::set_prop( step_node, "arrival_stop", arrival_str);
                        XML::set_prop( step_node, "trip", to_string( step->trip_id ) );
                    }
                    else if ( (*sit)->step_type == Roadmap::Step::GenericStep )
                    {
                        Roadmap::GenericStep* step = static_cast<Roadmap::GenericStep*>( *sit );
                        Multimodal::Edge* edge = static_cast<Multimodal::Edge*>( step );
                        
                        const Road::Graph* lroad_graph = 0;
                        const PublicTransport::Graph* pt_graph = 0;
                        db_id_t network_id = 0;
                        std::string stop_name, road_name;
                        std::string type_str = to_string( edge->connection_type());
                        if ( edge->connection_type() == Multimodal::Edge::Road2Transport ) {
                            lroad_graph = edge->source.road_graph;
                            pt_graph = edge->target.pt_graph;
                            stop_name = (*pt_graph)[edge->target.pt_vertex].name;
                            road_name = (*lroad_graph)[ (*pt_graph)[edge->target.pt_vertex].road_section].road_name;
                        }
                        else if ( edge->connection_type() == Multimodal::Edge::Transport2Road ) {
                            lroad_graph = edge->target.road_graph;
                            pt_graph = edge->source.pt_graph;
                            stop_name = (*pt_graph)[edge->source.pt_vertex].name;
                            road_name = (*lroad_graph)[ (*pt_graph)[edge->source.pt_vertex].road_section].road_name;
                        }
                        for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports.begin();
                              nit != graph_.public_transports.end();
                              ++nit ) {
                            if ( &nit->second == pt_graph ) {
                                network_id = nit->first;
                                break;
                            }
                        }
                        step_node = XML::new_node( "road_transport_step" );
                        
                        XML::set_prop( step_node, "type", type_str );
                        XML::set_prop( step_node, "road", road_name );                        
                        XML::set_prop( step_node, "network", graph_.network_map[ network_id ].name );
                        XML::set_prop( step_node, "stop", stop_name );
                    }
                    
                    for ( Tempus::Costs::iterator cit = gstep->costs.begin(); cit != gstep->costs.end(); cit++ )
                    {
                        xmlNode* cost_node = XML::new_node( "cost" );
                        XML::new_prop( cost_node,
                                       "type",
                                       to_string( cit->first ) );
                        XML::new_prop( cost_node,
                                       "value",
                                       to_string( cit->second ) );
                        XML::add_child( step_node, cost_node );
                    }

                    XML::set_prop( step_node, "wkb", (*sit)->geometry_wkb );

                    XML::add_child( result_node, step_node );
                }
            TIMING
                
                // total costs
                
                for ( Tempus::Costs::const_iterator cit = roadmap.total_costs.begin(); cit != roadmap.total_costs.end(); cit++ )
                {
                    xmlNode* cost_node = XML::new_node( "cost" );
                    XML::new_prop( cost_node,
                                   "type",
                                   to_string( cit->first ) );
                    XML::new_prop( cost_node,
                                   "value",
                                   to_string( cit->second ) );
                    XML::add_child( result_node, cost_node );
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
