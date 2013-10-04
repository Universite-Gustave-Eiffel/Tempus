#include <iostream>
#include <boost/format.hpp>

#include "wps_service.hh"
#include "multimodal_graph.hh"
#include "request.hh"
#include "roadmap.hh"
#include "db.hh"
#include "utils/graph_db_link.hh"

using namespace std;
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
	    std::string state_str = boost::lexical_cast<std::string>(a_state);
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
    Service * clone() const { return new PluginListService(*this);}
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
	    
	    xmlNode* root_node = XML::new_node( "plugins" );
	    std::vector<std::string> names( plugin_factory.plugin_list() );
	    for ( size_t i=0; i<names.size(); i++ )
	    {
		xmlNode* node = XML::new_node( "plugin" );
		XML::new_prop( node,
			       "name",
			       names[i] );
		XML::add_child( root_node, node );
	    }
	    output_parameters_[ "plugins" ] = root_node;
	    return output_parameters_;
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
    Service * clone() const { return new ConstantListService(*this);}
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
				  "  <xs:attribute name=\"need_network\" type=\"xs:int\"/>\n"
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
				  "  <xs:attribute name=\"provided_transport_types\" type=\"xs:long\"/>\n"
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
		    XML::new_prop( node, "need_network", it->second.need_network );
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
		    XML::new_prop( node, "provided_transport_types", it->second.provided_transport_types );
		    XML::add_child( root_node, node );
		}
		output_parameters_[ "transport_networks" ] = root_node;
	    }
	    
	    return output_parameters_;
	};
    };
    
    ///
    /// Base class for services that are linked to a particular plugin.
    /// Input var: plugin, the plugin name
    ///
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
    };

    ///
    /// "get_option_descriptions" service, get descriptions of a plugin options.
    ///
    /// Output var: outputs, lists of option with their name, type and description
    ///
    class GetOptionsDescService : public PluginService
    {
    public:
    Service * clone() const { return new GetOptionsDescService(*this);}
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
	    xmlNode* plugin_node = input_parameter_map["plugin"];
	    std::string plugin_str = XML::get_prop( plugin_node, "name" );
	    
	    xmlNode * options_node = XML::new_node( "options" );
	    Plugin::OptionDescriptionList options = plugin_factory.option_descriptions(plugin_str);
	    Plugin::OptionDescriptionList::iterator it;
	    for ( it = options.begin(); it != options.end(); it++ )
	    {
		xmlNode* option_node = XML::new_node( "option" );
		XML::new_prop( option_node, "name", it->first );
		XML::new_prop( option_node, "type",
			       boost::lexical_cast<std::string>( it->second.type ) );
		XML::new_prop( option_node, "description", it->second.description );
		XML::add_child( options_node, option_node );
	    }
	    
	    output_parameters_.clear();
	    output_parameters_[ "options" ] = options_node;
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
	//
	// Call to the stored procedure
	//
	string q = (boost::format( "SELECT tempus.road_node_id_from_coordinates(%1%, %2%)") % x % y).str();
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
    class SelectService : public PluginService, public Tempus::Request
    {
    public:
    Service * clone() const { return new SelectService(*this);}
	SelectService() : PluginService( "select" ), Tempus::Request()
	{
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

	    add_output_parameter( "results",
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
				  "<xs:complexType name=\"RoadTransportStep\">\n"
				  "  <xs:sequence>\n"
				  "    <xs:element name=\"type\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"road\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"network\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
				  "    <xs:element name=\"stop\" type=\"xs:string\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
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
				  "<xs:element name=\"results\">\n"
				  "  <xs:complexType>\n"
				  "    <xs:sequence>\n"
				  "    <xs:element name=\"result\" minOccurs=\"0\" maxOccurs=\"unbounded\">\n"
				  "      <xs:complexType>\n"
				  "        <xs:sequence>\n"
				  "          <xs:choice minOccurs=\"0\" maxOccurs=\"unbounded\">\n" // 0..N steps
				  "            <xs:element name=\"road_step\" type=\"RoadStep\"/>\n"
				  "            <xs:element name=\"public_transport_step\" type=\"PublicTransportStep\"/>\n"
				  "            <xs:element name=\"road_transport_step\" type=\"RoadTransportStep\"/>\n"
				  "          </xs:choice>\n"
				  "          <xs:element name=\"cost\" type=\"Cost\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>\n" // 0..N costs
				  "        </xs:sequence>\n"
				  "      </xs:complexType>\n"
				  "    </xs:element>\n"
				  "    </xs:sequence>\n"
				  "  </xs:complexType>\n"
				  "</xs:element>\n"
				  );
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

	void parse_constraint( xmlNode* node, Request::TimeConstraint& constraint )
	{
	    constraint.type = boost::lexical_cast<int>( XML::get_prop( node, "type") );
            
            std::string date_time = XML::get_prop( node, "date_time" );
	    const char* date_time_str = date_time.c_str();
	    int day, month, year, hour, min;
	    sscanf(date_time_str, "%04d-%02d-%02dT%02d:%02d", &year, &month, &day, &hour, &min );
	    constraint.date_time = boost::posix_time::ptime(boost::gregorian::date(year, month, day),
							    boost::posix_time::hours( hour ) + boost::posix_time::minutes( min ) );
	    
	}
	
	Service::ParameterMap& execute( ParameterMap& input_parameter_map )
	{
            output_parameters_.clear();
            // Ensure XML is OK
            Service::check_parameters( input_parameter_map, input_parameter_schema_ );
            ensure_minimum_state( Application::GraphBuilt );
            xmlNode* plugin_node = input_parameter_map["plugin"];
            const std::string plugin_str = XML::get_prop( plugin_node, "name" );
            std::auto_ptr<Plugin> plugin( plugin_factory.createPlugin( plugin_str ));
            // pre_process
            {
                
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
                this->optimizing_criteria.clear();
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
                this->allowed_networks.clear();
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
            }

            // process
            plugin->process();

            // result
            Tempus::Result& result = plugin->result();
            
            Multimodal::Graph& graph_ = Application::instance()->graph();
            Tempus::Road::Graph& road_graph = graph_.road;
            Db::Connection& db = Application::instance()->db_connection();
            
            xmlNode* root_node = XML::new_node( "results" );
            if ( result.size() == 0 )
            {
                output_parameters_["results"] = root_node;
                return output_parameters_;
            }

            Tempus::Result::const_iterator rit;
            for ( rit = result.begin(); rit != result.end(); ++rit ) {
                const Tempus::Roadmap& roadmap = *rit;
                
                xmlNode* result_node = XML::new_node( "result" );
                Roadmap::StepList::const_iterator sit;
                for ( sit = roadmap.steps.begin(); sit != roadmap.steps.end(); sit++ )
                {
                    xmlNode* step_node;
                    Roadmap::Step* gstep = *sit;
                    
                    xmlNode* wkb_node = XML::new_node( "wkb" );
                    XML::add_child( wkb_node, XML::new_text( (*sit)->geometry_wkb ));
                    
                    if ( (*sit)->step_type == Roadmap::Step::RoadStep )
                    {
                        Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *sit );
                        step_node = XML::new_node( "road_step" );
                        xmlNode* rs_node = XML::new_node( "road" );
                        string road_name = "Unknown road";
                        if ( edge_exists(step->road_section, road_graph) )
                            road_name = road_graph[ step->road_section].road_name;
                        XML::add_child( rs_node, XML::new_text( road_name ));
                        
                        xmlNode* end_movement_node = XML::new_node( "end_movement" );
                        XML::add_child( end_movement_node, XML::new_text( boost::lexical_cast<string>( step->end_movement ) ));
                        
                        XML::add_child( step_node, rs_node );
                        XML::add_child( step_node, end_movement_node );
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
                        
                        xmlNode* network_node = XML::new_node( "network" );
                        XML::add_child( network_node, XML::new_text( graph_.network_map[ step->network_id ].name ));
                        
                        xmlNode* departure_node = XML::new_node( "departure_stop" );
                        xmlNode* arrival_node = XML::new_node( "arrival_stop" );
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
                        XML::add_child( departure_node, XML::new_text( departure_str ));
                        XML::add_child( arrival_node, XML::new_text( arrival_str ));
                                                        
                        xmlNode* trip_node = XML::new_node( "trip" );
                        XML::add_child( trip_node, XML::new_text( boost::lexical_cast<string>( step->trip_id ) ));
                        XML::add_child( step_node, network_node );
                        XML::add_child( step_node, departure_node );
                        XML::add_child( step_node, arrival_node );
                        XML::add_child( step_node, trip_node );
                    }
                    else if ( (*sit)->step_type == Roadmap::Step::GenericStep )
                    {
                        Roadmap::GenericStep* step = static_cast<Roadmap::GenericStep*>( *sit );
                        Multimodal::Edge* edge = static_cast<Multimodal::Edge*>( step );
                        
                        const Road::Graph* road_graph = 0;
                        const PublicTransport::Graph* pt_graph = 0;
                        db_id_t network_id = 0;
                        std::string stop_name, road_name;
                        std::string type_str = boost::lexical_cast<std::string>( edge->connection_type());
                        if ( edge->connection_type() == Multimodal::Edge::Road2Transport ) {
                            road_graph = edge->source.road_graph;
                            pt_graph = edge->target.pt_graph;
                            stop_name = (*pt_graph)[edge->target.pt_vertex].name;
                            road_name = (*road_graph)[ (*pt_graph)[edge->target.pt_vertex].road_section].road_name;
                        }
                        else if ( edge->connection_type() == Multimodal::Edge::Transport2Road ) {
                            road_graph = edge->target.road_graph;
                            pt_graph = edge->source.pt_graph;
                            stop_name = (*pt_graph)[edge->source.pt_vertex].name;
                            road_name = (*road_graph)[ (*pt_graph)[edge->source.pt_vertex].road_section].road_name;
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
                        
                        xmlNode* type_node = XML::new_node( "type" );
                        XML::add_child( type_node, XML::new_text( type_str ));
                        
                        xmlNode* road_node = XML::new_node( "road" );
                        XML::add_child( road_node, XML::new_text( road_name ));
                        
                        xmlNode* network_node = XML::new_node( "network" );
                        XML::add_child( network_node, XML::new_text( graph_.network_map[ network_id ].name ));
                        
                        xmlNode* stop_node = XML::new_node( "stop" );
                        XML::add_child( stop_node, XML::new_text( stop_name ));

                        XML::add_child( step_node, type_node );
                        XML::add_child( step_node, road_node );
                        XML::add_child( step_node, network_node );
                        XML::add_child( step_node, stop_node );
                    }
                    
                    for ( Tempus::Costs::iterator cit = gstep->costs.begin(); cit != gstep->costs.end(); cit++ )
                    {
                        xmlNode* cost_node = XML::new_node( "cost" );
                        XML::new_prop( cost_node,
                                       "type",
                                       boost::lexical_cast<string>( cit->first ) );
                        XML::new_prop( cost_node,
                                       "value",
                                       boost::lexical_cast<string>( cit->second ) );
                        XML::add_child( step_node, cost_node );
                    }

                    XML::add_child( step_node, wkb_node );

                    XML::add_child( result_node, step_node );
                }
                
                // total costs
                
                for ( Tempus::Costs::const_iterator cit = roadmap.total_costs.begin(); cit != roadmap.total_costs.end(); cit++ )
                {
                    xmlNode* cost_node = XML::new_node( "cost" );
                    XML::new_prop( cost_node,
                                   "type",
                                   boost::lexical_cast<string>( cit->first ) );
                    XML::new_prop( cost_node,
                                   "value",
                                   boost::lexical_cast<string>( cit->second ) );
                    XML::add_child( result_node, cost_node );
                }

                XML::add_child( root_node, result_node );
            } // for each result
            output_parameters_[ "results" ] = root_node;

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
            
            output_parameters_[ "metrics" ] = metrics_node;
            return output_parameters_;
        }
    };

    static GetOptionsDescService get_option_desc_service;
    static PluginListService plugin_list_service;
    static SelectService select_service;
    static ConstantListService constant_list_service;

}; // WPS namespace
