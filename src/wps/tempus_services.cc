#include <iostream>
#include <boost/format.hpp>

#include "wps_service.hh"
#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"
#include "db.hh"

using namespace std;
using namespace Tempus;

namespace WPS
{
    class StateService : public Service
    {
    public:
	StateService() : Service("state")
	{
	    add_output_parameter( "state",
				  "<xs:element name=\"state\" type=\"xs:int\"/>\n" );
	    add_output_parameter( "db_options",
				  "<xs:element name=\"db_options\" type=\"xs:string\"/>\n" );
	}
	Service::ParameterMap& execute( Service::ParameterMap& input_parameter_map )
	{
	    output_parameters_.clear();
	    
	    int state = Application::instance()->state();
	    
	    xmlNode* root_node = xmlNewNode( NULL, (const xmlChar*)"state" );
	    xmlAddChild( root_node, xmlNewText( (const xmlChar*)( boost::lexical_cast<std::string>( state ).c_str() ) ) );
	    output_parameters_["state"] = root_node;
	    
	    xmlNode* options_node = xmlNewNode( NULL, (const xmlChar*)"db_options" );
	    xmlAddChild( options_node, xmlNewText( (const xmlChar*)Application::instance()->db_options().c_str() ) );
	    output_parameters_["db_options"] = options_node;
	    
	    return output_parameters_;
	}
    };
	
    void ensure_minimum_state( int state )
    {
	int a_state = Application::instance()->state();
	if ( a_state < state )
	{
	    std::string state_str = boost::lexical_cast<std::string>(a_state);
	    throw std::invalid_argument( "Invalid application state: " + state_str );
	}
    }

    class ConnectService : public Service
    {
    public:
	ConnectService() : Service("connect")
	{
	    add_input_parameter( "db_options",
				 "<xs:element name=\"db_options\" type=\"xs:string\"/>" );
	}
	
	Service::ParameterMap& execute( Service::ParameterMap& input_parameter_map )
	{
	    // Ensure XML is OK
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    
	    // now extract actual data
	    xmlNode* request_node = input_parameter_map["db_options"];
	    std::string db_options = (const char*)request_node->children->content;
	    
	    Application::instance()->connect( db_options );
	    output_parameters_.clear();
	    Application::instance()->state( Application::Connected );
	    return output_parameters_;
	}
    };

    class PreBuildService : public Service
    {
    public:
	PreBuildService() : Service("pre_build")
	{
	}
	Service::ParameterMap& execute( Service::ParameterMap& input_parameter_map )
	{
	    // Ensure XML is OK
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    
	    ensure_minimum_state( Application::Connected );
	    Application::instance()->pre_build_graph();
	    output_parameters_.clear();
	    Application::instance()->state( Application::GraphPreBuilt );
	    return output_parameters_;
	}
    };

    class BuildService : public Service
    {
    public:
	BuildService() : Service("build")
	{
	}
	Service::ParameterMap& execute( Service::ParameterMap& input_parameter_map )
	{
	    // Ensure XML is OK
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    
	    ensure_minimum_state( Application::GraphPreBuilt );
	    Application::instance()->build_graph();
	    output_parameters_.clear();
	    Application::instance()->state( Application::GraphBuilt );
	    return output_parameters_;
	}
    };

    class PluginListService : public Service
    {
    public:
	PluginListService() : Service("plugin_list")
	{
	    add_output_parameter( "plugins",
				  "<xs:complexType name=\"Plugin\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"plugins\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "      <xs:element name=\"plugin\" type=\"Plugin\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
	};
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    output_parameters_.clear();
	    
	    xmlNode* root_node = xmlNewNode( NULL, (const xmlChar*)"plugins" );
	    Plugin::PluginList::iterator it;
	    for ( it = Plugin::plugin_list().begin(); it != Plugin::plugin_list().end(); it++ )
	    {
		xmlNode* node = xmlNewNode( NULL, (const xmlChar*)"plugin" );
		xmlNewProp( node,
			    (const xmlChar*)"name",
			    (const xmlChar*)(it->first.c_str()) );
		xmlAddChild( root_node, node );
	    }
	    output_parameters_[ "plugins" ] = root_node;
	    return output_parameters_;
	};
    };

    class ConstantListService : public Service
    {
    public:
	ConstantListService() : Service("constant_list")
	{
	    add_output_parameter( "road_types",
				  "<xs:complexType name=\"RoadType\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"id\" type=\"xs:long\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"road_types\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "      <xs:element name=\"road_type\" type=\"RoadType\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
	    add_output_parameter( "transport_types",
				  "<xs:complexType name=\"TransportType\">\n"
				  "  <xs:attribute name=\"id\" type=\"xs:long\"/>\n"
				  "  <xs:attribute name=\"parent_id\" type=\"xs:long\"/>\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"need_parking\" type=\"xs:int\"/>\n"
				  "  <xs:attribute name=\"need_station\" type=\"xs:int\"/>\n"
				  "  <xs:attribute name=\"need_return\" type=\"xs:int\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"transport_types\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "      <xs:element name=\"transport_type\" type=\"TransportType\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
	    add_output_parameter( "transport_networks",
				  "<xs:complexType name=\"TransportNetwork\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"id\" type=\"xs:long\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"transport_networks\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "      <xs:element name=\"transport_network\" type=\"TransportNetwork\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
	};
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    output_parameters_.clear();
	    
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
		output_parameters_[ "road_types" ] = root_node;
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
		    XML::add_child( root_node, node );
		}
		output_parameters_[ "transport_types" ] = root_node;
	    }

	    {
		xmlNode* root_node = XML::new_node( "transport_networks" );
		Multimodal::Graph::NetworkMap::iterator it;
		for ( it = graph.network_map.begin(); it != graph.network_map.end(); it++ )
		{
		    xmlNode* node = XML::new_node( "transport_network" );
		    XML::new_prop( node, "id", it->first );
		    XML::new_prop( node, "name", it->second.name );
		    XML::add_child( root_node, node );
		}
		output_parameters_[ "transport_networks" ] = root_node;
	    }

	    return output_parameters_;
	};
    };

    class PluginService : public Service
    {
    public:
	PluginService( const std::string& name ) : Service( name )
	{
	    add_input_parameter( "plugin",
				 "<xs:complexType name=\"Plugin\">\n"
				 "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				 "</xs:complexType>\n"
				 "<xs:element name=\"plugin\" type=\"Plugin\"/>\n" );
	}
	Plugin* get_plugin( ParameterMap& input_parameters )
	{
	    xmlNode* plugin_node = input_parameters["plugin"];
	    std::string plugin_str = (const char*)( xmlGetProp( plugin_node, (const xmlChar*)"name" ) );
	    Plugin::PluginList::iterator it = Plugin::plugin_list().find( plugin_str );
	    if ( it == Plugin::plugin_list().end() )
		throw std::invalid_argument( "Plugin " + plugin_str + " is not loaded" );
	    return it->second;
	}
    };

    class GetOptionsDescService : public PluginService
    {
    public:
	GetOptionsDescService() : PluginService("get_option_descriptions")
	{
	    add_output_parameter( "options",
				  "<xs:complexType name=\"Option\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"type\" type=\"xs:int\"/>\n"
				  "  <xs:attribute name=\"description\" type=\"xs:string\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:complexType name=\"Options\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"option\" type=\"Option\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "  </xs:sequence>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"options\" type=\"Options\"/>\n" );
	}
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    
	    xmlNode * options_node = xmlNewNode( NULL, (const xmlChar*)"options" );
	    Plugin::OptionDescriptionList options = plugin->option_descriptions();
	    Plugin::OptionDescriptionList::iterator it;
	    for ( it = options.begin(); it != options.end(); it++ )
	    {
		xmlNode* option_node = xmlNewNode( NULL, (const xmlChar*)"option" );
		xmlNewProp( option_node,
			    (const xmlChar*)"name",
			    (const xmlChar*)( it->first.c_str() ) );
		xmlNewProp( option_node,
			    (const xmlChar*)"type",
			    (const xmlChar*)( boost::lexical_cast<std::string>( it->second.type ).c_str() ) );
		xmlNewProp( option_node,
			    (const xmlChar*)"description",
			    (const xmlChar*)( it->second.description.c_str() ) );
		xmlAddChild( options_node, option_node );
	    }
	    
	    output_parameters_.clear();
	    output_parameters_[ "options" ] = options_node;
	    return output_parameters_;
	}
    };

    class GetMetricsService : public PluginService
    {
    public:
	GetMetricsService() : PluginService("get_metrics")
	{
	    add_output_parameter( "metrics",
				  "<xs:complexType name=\"Metric\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"value\" type=\"xs:string\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:complexType name=\"Metrics\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"metric\" type=\"Metric\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "  </xs:sequence>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"metrics\" type=\"Metrics\"/>\n" );
	}
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    
	    xmlNode * metrics_node = xmlNewNode( NULL, (const xmlChar*)"metrics" );
	    Plugin::MetricValueList metrics = plugin->metrics();
	    Plugin::MetricValueList::iterator it;
	    for ( it = metrics.begin(); it != metrics.end(); it++ )
	    {
		xmlNode* metric_node = xmlNewNode( NULL, (const xmlChar*)"metric" );
		xmlNewProp( metric_node,
			    (const xmlChar*)"name",
			    (const xmlChar*)( it->first.c_str() ) );
		xmlNewProp( metric_node,
			    (const xmlChar*)"value",
			    (const xmlChar*)( plugin->metric_to_string( it->first ).c_str() ) );
		
		xmlAddChild( metrics_node, metric_node );
	    }
	    
	    output_parameters_.clear();
	    output_parameters_[ "metrics" ] = metrics_node;
	    return output_parameters_;
	}
    };

    class GetOptionsService : public PluginService
    {
    public:
	GetOptionsService() : PluginService("get_options")
	{
	    add_output_parameter( "options",
				  "<xs:complexType name=\"Option\">\n"
				  "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"value\" type=\"xs:string\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:complexType name=\"Options\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"option\" type=\"Option\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  "  </xs:sequence>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"options\" type=\"Options\"/>\n" );
	}
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    
	    xmlNode * options_node = xmlNewNode( NULL, (const xmlChar*)"options" );
	    Plugin::OptionValueList options = plugin->options();
	    Plugin::OptionValueList::iterator it;
	    for ( it = options.begin(); it != options.end(); it++ )
	    {
		xmlNode* option_node = xmlNewNode( NULL, (const xmlChar*)"option" );
		xmlNewProp( option_node,
			    (const xmlChar*)"name",
			    (const xmlChar*)( it->first.c_str() ) );
		xmlNewProp( option_node,
			    (const xmlChar*)"value",
			    (const xmlChar*)( plugin->option_to_string( it->first ).c_str() ) );
		
		xmlAddChild( options_node, option_node );
	    }
	    
	    output_parameters_.clear();
	    output_parameters_[ "options" ] = options_node;
	    return output_parameters_;
	}
    };

    class SetOptionsService : public PluginService
    {
    public:
	SetOptionsService() : PluginService("set_options")
	{
	    add_input_parameter( "options",
				 "<xs:complexType name=\"Option\">\n"
				 "  <xs:attribute name=\"name\" type=\"xs:string\"/>\n"
				 "  <xs:attribute name=\"value\" type=\"xs:string\"/>\n"
				 "</xs:complexType>\n"
				 "<xs:complexType name=\"Options\">\n"
				 "  <xs:sequence>\n"
				 "    <xs:element name=\"option\" type=\"Option\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				 "  </xs:sequence>\n"
				 "</xs:complexType>\n"
				 "<xs:element name=\"options\" type=\"Options\"/>\n" );
	}
	
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    
	    xmlNode *options_node = input_parameter_map[ "options" ];
	    xmlNode *option_node = options_node->children;
	    
	    while ( option_node = XML::get_next_nontext( option_node ) )
	    {
		string name = (const char*)xmlGetProp( option_node, (const xmlChar*)"name" );
		string value = (const char*)xmlGetProp( option_node, (const xmlChar*)"value" );
		plugin->set_option_from_string( name, value );
		option_node = option_node->next;
	    }
	    
	    output_parameters_.clear();
	    return output_parameters_;
	}
    };
    
    void get_xml_point( xmlNode* node, double& x, double& y )
    {
	xmlNode* x_node = XML::get_next_nontext( node->children );
	xmlNode* y_node = XML::get_next_nontext( x_node->next );
	
	string x_str = (const char*)x_node->children->content;
	string y_str = (const char*)y_node->children->content;
	
	x = boost::lexical_cast<double>( x_str );
	y = boost::lexical_cast<double>( y_str );
    }
    
    Tempus::db_id_t road_vertex_id_from_coordinates( Db::Connection& db, double x, double y )
    {
	string q = (boost::format( "SELECT tempus.road_node_id_from_coordinates(%1%, %2%)") % x % y).str();
	Db::Result res = db.exec( q );
	if ( res.size() == 0 )
	    return 0;
	return res[0][0].as<Tempus::db_id_t>();
    }
        
    class PreProcessService : public PluginService, public Tempus::Request
    {
    public:
	PreProcessService() : PluginService("pre_process"), Tempus::Request()
	{
	    // define the XML schema of input parameters
	    add_input_parameter( "request",
				 "  <xs:complexType name=\"TimeConstraint\">\n"
				 "    <xs:attribute name=\"type\" type=\"xs:int\"/>\n"
				 "    <xs:attribute name=\"date_time\" type=\"xs:dateTime\"/>\n"
				 "  </xs:complexType>\n"
				 "  <xs:complexType name=\"Point\">\n"
				 "    <xs:sequence>\n"
				 "      <xs:element name=\"x\" type=\"xs:float\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"y\" type=\"xs:float\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "    </xs:sequence>\n"
				 "  </xs:complexType>\n"
				 "  <xs:complexType name=\"Step\">\n"
				 "    <xs:sequence>\n"
				 "      <xs:element name=\"destination\" type=\"Point\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"constraint\" type=\"TimeConstraint\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"private_vehicule_at_destination\" type=\"xs:boolean\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "    </xs:sequence>\n"
				 "  </xs:complexType>\n"
				 "  <xs:complexType name=\"Request\">\n"
				 "    <xs:sequence>\n"
				 "      <xs:element name=\"origin\" type=\"Point\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"departure_constraint\" type=\"TimeConstraint\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"parking_location\" type=\"Point\" minOccurs=\"0\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"optimizing_criterion\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"unbounded\"/>\n"
				 "      <xs:element name=\"allowed_transport_types\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				 "      <xs:element name=\"allowed_network\" type=\"xs:long\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				 "      <xs:element name=\"step\" type=\"Step\" minOccurs=\"1\" maxOccurs=\"unbounded\"/>\n"
				 "    </xs:sequence>\n"
				 "  </xs:complexType>\n"
				 "<xs:element name=\"request\" type=\"Request\"/>\n"
				 );
	}

	void parse_constraint( xmlNode* node, Request::TimeConstraint& constraint )
	{
	    const char * type_str = (const char*)xmlGetProp( node, (const xmlChar*)"type" );
	    constraint.type = boost::lexical_cast<int>( type_str );

	    const char* date_time_str = (const char*)xmlGetProp( node, (const xmlChar*)"date_time" );
	    int day, month, year, hour, min;
	    sscanf(date_time_str, "%04d-%02d-%02dT%02d:%02d", &year, &month, &day, &hour, &min );
	    constraint.date_time = boost::posix_time::ptime(boost::gregorian::date(year, month, day),
							    boost::posix_time::hours( hour ) + boost::posix_time::minutes( min ) );

	}

	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    // Ensure XML is OK
	    Service::check_parameters( input_parameter_map, input_parameter_schema_ );
	    ensure_minimum_state( Application::GraphBuilt );
	    
	    Plugin* plugin = get_plugin( input_parameter_map );
	    double x,y;
	    Db::Connection& db = Application::instance()->db_connection();
	    
	    // now extract actual data
	    xmlNode* request_node = input_parameter_map["request"];
	    xmlNode* field = XML::get_next_nontext( request_node->children );
	    
	    Tempus::Road::Graph& road_graph = Application::instance()->graph().road;
	    get_xml_point( field, x, y);
	    Tempus::db_id_t origin_id = road_vertex_id_from_coordinates( db, x, y );
	    if ( origin_id == 0 )
	    {
		throw std::invalid_argument( "Cannot find origin_id" );
	    }
	    this->origin = vertex_from_id( origin_id, road_graph );
	    if ( this->origin == Road::Vertex() )
	    {
		throw std::invalid_argument( "Cannot find origin vertex" );
	    }
	    
	    // departure_constraint
	    field = XML::get_next_nontext( field->next );
	    parse_constraint( field, this->departure_constraint );
	    
	    // parking location id, optional
	    xmlNode *n = XML::get_next_nontext( field->next );
	    if ( !xmlStrcmp( n->name, (const xmlChar*)"parking_location" ) )
	    {
		get_xml_point( n, x, y );
		Tempus::db_id_t parking_id = road_vertex_id_from_coordinates( db, x, y );
		if ( parking_id == 0 )
		{
		    throw std::invalid_argument( "Cannot find parking_id" );
		}
		this->parking_location = vertex_from_id( parking_id, road_graph );
		field = n;
	    }
	    
	    // optimizing criteria
	    field = XML::get_next_nontext( field->next );
	    this->optimizing_criteria.push_back( boost::lexical_cast<int>( field->children->content ) );
	    field = XML::get_next_nontext( field->next );	
	    while ( !xmlStrcmp( field->name, (const xmlChar*)"optimizing_criterion" ) )
	    {
		this->optimizing_criteria.push_back( boost::lexical_cast<int>( field->children->content ) );
		field = XML::get_next_nontext( field->next );	
	    }
	    
	    // allowed transport types
	    this->allowed_transport_types = boost::lexical_cast<int>( field->children->content );
	
	    // allowed networks, 1 .. N
	    field = XML::get_next_nontext( field->next );
	    while ( !xmlStrcmp( field->name, (const xmlChar *)"allowed_network" ) )
	    {
		Tempus::db_id_t network_id = boost::lexical_cast<Tempus::db_id_t>(field->children->content);
		this->allowed_networks.push_back( network_id );
		field = XML::get_next_nontext( field->next );
	    }
	
	    // steps, 1 .. N
	    steps.clear();
	    while ( field )
	    {
		this->steps.resize( steps.size() + 1 );
		
		xmlNode *subfield;
		// destination id
		subfield = XML::get_next_nontext( field->children );
		get_xml_point( subfield, x, y);
		Tempus::db_id_t destination_id = road_vertex_id_from_coordinates( db, x, y );
		if ( destination_id == 0 )
		{
		    throw std::invalid_argument( "Cannot find origin_id" );
		}
		this->steps.back().destination = vertex_from_id( destination_id, road_graph );

		// constraint
		subfield = XML::get_next_nontext( subfield->next );
		parse_constraint( subfield, this->steps.back().constraint );
	    
		// private_vehicule_at_destination
		subfield = XML::get_next_nontext( subfield->next );
		string val = (const char*)subfield->children->content;
		this->steps.back().private_vehicule_at_destination = ( val == "true" );
	    
		// next step
		field = XML::get_next_nontext( field->next ); 
	    }

	    // call cycle
	    plugin->cycle();

	    // then call pre_process
	    plugin->pre_process( *this );
	    output_parameters_.clear();
	    return output_parameters_;
	}
    };

    class ProcessService : public PluginService
    {
    public:
	ProcessService() : PluginService("process")
	{
	}
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    ensure_minimum_state( Application::GraphBuilt );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    plugin->process();
	    output_parameters_.clear();
	    return output_parameters_;
	}
    };
	
    class ResultService : public PluginService
    {
    public:
	ResultService() : PluginService( "result" )
	{
	    add_output_parameter( "result",
				  "<xs:complexType name=\"DbId\">\n"
				  "  <xs:attribute name=\"id\" type=\"xs:long\"/>\n" // db_id
				  "</xs:complexType>\n"
				  "  <xs:complexType name=\"Point\">\n"
				  "    <xs:sequence>\n"
				  "      <xs:element name=\"x\" type=\"xs:float\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "      <xs:element name=\"y\" type=\"xs:float\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "<xs:complexType name=\"Cost\">\n"
				  "  <xs:attribute name=\"type\" type=\"xs:string\"/>\n"
				  "  <xs:attribute name=\"value\" type=\"xs:float\"/>\n"
				  "</xs:complexType>\n"
				  "<xs:complexType name=\"RoadStep\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"road\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"end_movement\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"cost\" type=\"Cost\" maxOccurs=\"unbounded\"/>\n" // 0..N costs
				  "    <xs:element name=\"wkb\" type=\"xs:string\"/>\n"
				  "  </xs:sequence>\n"
				  "</xs:complexType>\n"
				  "<xs:complexType name=\"PublicTransportStep\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"network\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"departure_stop\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"arrival_stop\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"trip\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"cost\" type=\"Cost\" maxOccurs=\"unbounded\"/>\n" // 0..N costs
				  "    <xs:element name=\"wkb\" type=\"xs:string\"/>\n"
				  "  </xs:sequence>\n"
				  "</xs:complexType>\n"
				  "<xs:element name=\"result\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "      <xs:choice minOccurs=\"0\" maxOccurs=\"unbounded\">\n" // 0..N steps
				  "        <xs:element name=\"road_step\" type=\"RoadStep\"/>\n"
				  "        <xs:element name=\"public_transport_step\" type=\"PublicTransportStep\"/>\n"
				  "      </xs:choice>\n"
				  "      <xs:element name=\"cost\" type=\"Cost\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n" // 0..N costs
				  //				  "      <xs:element name=\"overview_path\" minOccurs=\"0\" maxOccurs=\"1\">\n" // 0..1
				  //				  "        <xs:complexType>\n"
				  //				  "          <xs:sequence>\n"
				  //				  "            <xs:element name=\"node\" type=\"Point\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n"
				  //				  "          </xs:sequence>\n"
				  //				  "        </xs:complexType>\n"
				  //				  "      </xs:element>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
	}
	
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    ensure_minimum_state( Application::GraphBuilt );
	    output_parameters_.clear();
	    Plugin* plugin = get_plugin( input_parameter_map );
	    Tempus::Result& result = plugin->result();
	    
	    Multimodal::Graph& graph_ = Application::instance()->graph();
	    Tempus::Road::Graph& road_graph = graph_.road;
	    Db::Connection& db = Application::instance()->db_connection();
	    
	    xmlNode* root_node = xmlNewNode( /*ns=*/ NULL, (const xmlChar*)"result" );
	    if ( result.size() == 0 )
	    {
		output_parameters_["result"] = root_node;
		return output_parameters_;
	    }
	    Tempus::Roadmap& roadmap = result.back();
#if 0
	    xmlNode* overview_path_node = xmlNewNode( /* ns = */ NULL,
						      (const xmlChar*)"overview_path" );
	    Roadmap::PointList::iterator it;
	    for ( it = roadmap.overview_path.begin(); it != roadmap.overview_path.end(); it++ )
	    {
		xmlNode *node = xmlNewNode( /* ns = */ NULL, (const xmlChar*)"node" );
		xmlNode *x_node = xmlNewNode( NULL, (const xmlChar*)"x" );
		xmlNode *y_node = xmlNewNode( NULL, (const xmlChar*)"y" );
		xmlAddChild( x_node, xmlNewText( (const xmlChar*)((boost::lexical_cast<string>(it->x)).c_str()) ) );
		xmlAddChild( y_node, xmlNewText( (const xmlChar*)((boost::lexical_cast<string>(it->y)).c_str()) ) );
		xmlAddChild( node, x_node );
		xmlAddChild( node, y_node );
		xmlAddChild( overview_path_node, node );
	    }
#endif	    
	    Roadmap::StepList::iterator sit;
	    for ( sit = roadmap.steps.begin(); sit != roadmap.steps.end(); sit++ )
	    {
		xmlNode* step_node;
		Roadmap::Step* gstep = *sit;

		xmlNode* wkb_node = xmlNewNode( NULL, (const xmlChar*)"wkb" );
		xmlAddChild( wkb_node, xmlNewText((const xmlChar*)((*sit)->geometry_wkb.c_str())) );

		if ( (*sit)->step_type == Roadmap::Step::RoadStep )
		{
		    Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *sit );
		    step_node = xmlNewNode( NULL, (const xmlChar*)"road_step" );
		    xmlNode* rs_node = xmlNewNode( NULL, (const xmlChar*)"road" );
		    string road_name = "Unknown road";
		    if ( edge_exists(step->road_section, road_graph) )
			road_name = road_graph[ step->road_section].road_name;
		    xmlAddChild( rs_node, xmlNewText((const xmlChar*)road_name.c_str()) );
		    
		    xmlNode* end_movement_node = xmlNewNode( NULL, (const xmlChar*)"end_movement" );
		    xmlAddChild( end_movement_node, xmlNewText((const xmlChar*)(boost::lexical_cast<string>( step->end_movement )).c_str()) );
		    
		    xmlAddChild( step_node, rs_node );
		    xmlAddChild( step_node, end_movement_node );
		}
		else if ( (*sit)->step_type == Roadmap::Step::PublicTransportStep )
		{
		    Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( *sit );

		    if ( graph_.public_transports.find( step->network_id ) == graph_.public_transports.end() )
		    {
			// can't find the pt network
		    }
		    PublicTransport::Graph& pt_graph = graph_.public_transports[ step->network_id ];

		    step_node = xmlNewNode( NULL, (const xmlChar*)"public_transport_step" );
		    
		    xmlNode* network_node = xmlNewNode( NULL, (const xmlChar*)"network" );
		    xmlAddChild( network_node, xmlNewText((const xmlChar*)(graph_.network_map[ step->network_id ].name.c_str())) );

		    xmlNode* departure_node = xmlNewNode( NULL, (const xmlChar*)"departure_stop" );
		    xmlNode* arrival_node = xmlNewNode( NULL, (const xmlChar*)"arrival_stop" );
		    string departure_str;
		    PublicTransport::Vertex v1 = vertex_from_id( pt_graph[step->section].stop_from, pt_graph );
		    PublicTransport::Vertex v2 = vertex_from_id( pt_graph[step->section].stop_to, pt_graph );
		    if ( vertex_exists( v1, pt_graph ) )
		    {
			departure_str = pt_graph[ v1 ].name;
		    }
		    string arrival_str;
		    if ( vertex_exists( v2, pt_graph ) )
		    {
			arrival_str = pt_graph[ v2 ].name;
		    }
		    xmlAddChild( departure_node, xmlNewText((const xmlChar*)(departure_str.c_str())) );
		    xmlAddChild( arrival_node, xmlNewText((const xmlChar*)(arrival_str.c_str())) );

		    xmlNode* trip_node = xmlNewNode( NULL, (const xmlChar*)"trip" );
		    xmlAddChild( trip_node, xmlNewText((const xmlChar*)(boost::lexical_cast<string>( step->trip_id ).c_str())) );
		    xmlAddChild( step_node, network_node );
		    xmlAddChild( step_node, departure_node );
		    xmlAddChild( step_node, arrival_node );
		    xmlAddChild( step_node, trip_node );
		}

		for ( Tempus::Costs::iterator cit = gstep->costs.begin(); cit != gstep->costs.end(); cit++ )
		{
		    xmlNode* cost_node = xmlNewNode( NULL, (const xmlChar*)"cost" );
		    xmlNewProp( cost_node,
				(const xmlChar*)"type",
				(const xmlChar*)(boost::lexical_cast<string>( cit->first )).c_str() );
		    xmlNewProp( cost_node,
				(const xmlChar*)"value",
				(const xmlChar*)(boost::lexical_cast<string>( cit->second )).c_str() );
		    xmlAddChild( step_node, cost_node );
		}

		xmlAddChild( step_node, wkb_node );

		xmlAddChild( root_node, step_node );
	    }
	    //	    xmlAddChild( root_node, overview_path_node );
	    
	    output_parameters_[ "result" ] = root_node;
	    return output_parameters_;
	}
    };

    class CleanupService : public PluginService
    {
    public:
	CleanupService() : PluginService("cleanup") {}
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    ensure_minimum_state( Application::GraphBuilt );
	    Plugin* plugin = get_plugin( input_parameter_map );
	    plugin->cleanup();
	    output_parameters_.clear();
	    return output_parameters_;
	}
    };

    static StateService state_service_;
    static GetMetricsService get_metrics_service;
    static GetOptionsService get_options_service;
    static GetOptionsDescService get_option_desc_service;
    static SetOptionsService set_option_service;
    static ConnectService connect_service_;
    static PluginListService plugin_list_service;
    static PreBuildService pre_build_service_;
    static BuildService build_service_;
    static PreProcessService pre_process_service_;
    static ProcessService process_service_;
    static ResultService result_service_;
    static CleanupService cleanup_service_;
    static ConstantListService constant_list_service;

}; // WPS namespace
