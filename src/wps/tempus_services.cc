#include <iostream>

#include "tempus_services.hh"

using namespace std;

namespace WPS
{    
    ServiceWithRequest::ServiceWithRequest( const std::string& name ) : Service(name), Tempus::Request()
    {
	// define the XML schema of input parameters
	add_input_parameter( "request",
			     "  <xs:complexType name=\"TimeConstraint\">\n"
			     "    <xs:attribute name=\"type\" type=\"xs:int\"/>\n"
			     "    <xs:attribute name=\"date_time\" type=\"xs:dateTime\"/>\n"
			     "  </xs:complexType>\n"
			     "  <xs:complexType name=\"Step\">\n"
			     "    <xs:sequence>\n"
			     "      <xs:element name=\"destination_id\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"constraint\" type=\"TimeConstraint\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"private_vehicule_at_destination\" type=\"xs:boolean\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "    </xs:sequence>\n"
			     "  </xs:complexType>\n"
			     "  <xs:complexType name=\"Request\">\n"
			     "    <xs:sequence>\n"
			     "      <xs:element name=\"origin_id\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"departure_constraint\" type=\"TimeConstraint\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"parking_location_id\" type=\"xs:int\" minOccurs=\"0\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"optimizing_criterion\" type=\"xs:int\" minOccurs=\"1\"/>\n"
			     "      <xs:element name=\"allowed_transport_types\" type=\"xs:int\" minOccurs=\"1\" maxOccurs=\"1\"/>\n"
			     "      <xs:element name=\"allowed_network\" type=\"xs:int\" minOccurs=\"0\"/>\n"
			     "      <xs:element name=\"step\" type=\"Step\" minOccurs=\"1\"/>\n"
			     "    </xs:sequence>\n"
			     "  </xs:complexType>\n"
			     "<xs:element name=\"request\" type=\"Request\"/>\n"
			     );
    }
    
    void ServiceWithRequest::parse_xml_parameters( InputParameterMap& input_parameter_map )
    {
	// Ensure XML is OK
	Service::check_parameters( input_parameter_map );
	
	// now extract actual data
	xmlNode* request_node = input_parameter_map["request"];
	cout << "request_node " << request_node->name << endl;
	xmlNode* field = XML::get_next_nontext( request_node->children );
	
	string origin_str = (const char*)field->children->content;
	cout << "origin_str = " << origin_str << endl;
	this->origin = boost::lexical_cast<int>( origin_str );
	cout << "origin " << origin << endl;
	
	// departure_constraint
	// TODO
	field = XML::get_next_nontext( field->next );
	
	// parking location id, optional
	xmlNode *n = XML::get_next_nontext( field->next );
	if ( !xmlStrcmp( n->name, (const xmlChar*)"parking_location_id" ) )
	{
	    this->parking_location = boost::lexical_cast<int>( n->children->content );
	    field = n;
	    cout << "parking_location " << parking_location << endl;
	}
	
	// allowed transport types
	field = XML::get_next_nontext( field->next );	
	this->allowed_transport_types = boost::lexical_cast<int>( field->children->content );
	cout << "allowed transport types " << allowed_transport_types << endl;
	
	// allowed networks, 1 .. N
	field = XML::get_next_nontext( field->next );
	while ( !xmlStrcmp( field->name, (const xmlChar *)"allowed_network" ) )
	{
	    this->allowed_networks.push_back( boost::lexical_cast<int>(field->children->content) );
	    field = XML::get_next_nontext( field->next );
	    cout << "allowed network " << allowed_networks.size() << endl;
	}
	
	// steps, 1 .. N
	field = XML::get_next_nontext( field->next );
	steps.clear();
	while ( field )
	{
	    cout << "field " << field->name << endl;
	    this->steps.resize( steps.size() + 1 );
		
	    xmlNode *subfield;
	    // destination id
	    subfield = XML::get_next_nontext( field->children );
	    this->steps.back().destination = boost::lexical_cast<int>(subfield->children->content);
	    cout << "destination " << steps.back().destination << endl;
	    
	    // constraint
	    // TODO
	    subfield = XML::get_next_nontext( subfield->next );
	    
	    // private_vehicule_at_destination
	    subfield = XML::get_next_nontext( subfield->next );
	    string val = (const char*)subfield->children->content;
	    this->steps.back().private_vehicule_at_destination = ( val == "true" );
	    cout << "pvat " << steps.back().private_vehicule_at_destination << endl;
	    
	    // next step
	    field = XML::get_next_nontext( field->next ); 
	}
    }
}; // WPS namespace
