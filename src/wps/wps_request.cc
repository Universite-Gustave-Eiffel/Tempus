#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "wps_request.hh"
#include "wps_service.hh"
#include "xml_helper.hh"

using namespace std;

namespace WPS {

    int Request::print_error_status( int status, const std::string& msg )
    {
	outs_ << "Status: " << status << " " << msg << endl;
	outs_ << "Content-type: text/html" << endl;
	outs_ << endl;
	outs_ << "<h2>" + msg + "</h2>" << endl;
	return status;
    }
    
    int Request::print_exception( const std::string& type, const std::string& message )
    {
	string escaped_msg = XML::escape_text( message );

	outs_ << "Status: 400" << endl;
	outs_ << "Content-type: text/xml" << endl;
	outs_ << endl;
	outs_ << "<ows:ExceptionReport xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd\" version=\"1.0.0\" xml:lang=\"en-US\">" << endl;
	outs_ << "<ows:Exception exceptionCode=\"" + type + "\"><ows:ExceptionText>" + escaped_msg + "</ows:ExceptionText></ows:Exception>" << endl;
	outs_ << "</ows:ExceptionReport>" << endl;
	return 400;
    }

    int Request::process()
    {
	if ( getParam("REQUEST_METHOD") == 0 )
	{
	   CERR << "This program is intended to be called from a web server, as a CGI" << endl;
	    return 1;
	}
	string request_method = getParam( "REQUEST_METHOD" );
    
	// libxml inits
	scoped_xmlDoc xml_doc;

	// Local map that stores request parameters :
	// Service
	// Version
	// Request
	// Identifier
	map<string, string> query;
	if ( request_method == "GET" )
	{
	    string query_string = getParam( "QUERY_STRING" );
	    //
	    // Parse GET parameters
	    vector<string> strs;
	    boost::split( strs, query_string, boost::is_any_of("&") );
	    for ( size_t i = 0; i < strs.size(); i++ )
	    {
		size_t idx = strs[i].find('=');
		if ( idx == string::npos )
		{
		    query[boost::to_lower_copy(strs[i])] = "";
		}
		else
		{
		    string key = boost::to_lower_copy(strs[i].substr(0, idx));
		    query[key] = strs[i].substr( idx+1 );
		}
	    }
	}
	else if ( request_method == "POST" )
	{
	    // Data should be in text/xml
	    if ( string(getParam( "CONTENT_TYPE" )) != "text/xml" )
	    {
		return print_error_status( 406, "Wrong content-type. Must be text/xml" );
	    }

	    // The root XML element must be the name of the operation	
	    string line;
	    string doc = "";
	    while ( !ins_.eof() )
	    {
		char c;
		ins_.get(c);
		if (ins_.eof())
		    break;
		doc.push_back(c);
	    }
            //CERR << doc << endl;
	
	    xml_doc = xmlReadMemory( doc.c_str(), doc.size(), "query.xml", NULL, 0 );
	    if ( xml_doc.get() == NULL )
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "Malformed XML request: " + string(xmlGetLastError()->message) );
	    }

	    xmlNodePtr root = xmlDocGetRootElement( xml_doc.get() );
	    query["request"] = (const char*)root->name;

	    xmlChar* service = xmlGetProp( root, (const xmlChar*)"service" );
	    if ( service == NULL )
		query["service"] = "";
	    else
		query["service"] = (const char*)service;

	    xmlChar* version = xmlGetProp( root, (const xmlChar*)"version" );
	    if ( version == NULL )
		query["version"] = "";
	    else
		query["version"] = (const char*)version;
	}


	if ( boost::to_lower_copy(query["service"]) != "wps" )
	{
	    return print_error_status( 400, "Only 'wps' service is supported." );
	}
	if ( query["version"] != "1.0.0" )
	{
	    return print_error_status( 400, "Only '1.0.0' version is supported" );
	}

	string wps_request = query["request"];
	if ( wps_request == "GetCapabilities" )
	{
	    if ( request_method != "GET" )
	    {
		return print_error_status( 405, "Method not allowed" );
	    }

	    outs_ << "Content-type: text/xml" << endl;
	    outs_ << endl;

	    WPS::Service::get_xml_capabilities( outs_, getParam("SCRIPT_NAME") );
	    outs_.flush();
	}
	else if ( wps_request == "DescribeProcess" )
	{
	    if ( request_method != "GET" )
	    {
		return print_error_status( 405, "Method not allowed" );
	    }

        WPS::Service* service = WPS::Service::get_service( query["identifier"] );
	    if ( !service )
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "Unknown identifier" );
	    }
	    outs_ << "Content-type: text/xml" << endl;
	    outs_ << endl;

	    service->get_xml_description( outs_ );

	    outs_.flush();
	}
	else if ( wps_request == "Execute" )
	{
	    if ( request_method != "POST" )
	    {
		return print_error_status( 405, "Method not allowed" );
	    }

	    xmlNode* root = xmlDocGetRootElement( xml_doc.get() );
	    xmlNode* child = XML::get_next_nontext( root->children );

	    string identifier;
	    if ( child && !xmlStrcmp(child->name, (const xmlChar*)"Identifier") )
	    {
		if ( child->children && child->children->content )
		    identifier = (const char *)child->children->content;
	    }
	    if ( identifier == "" )
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "Identifier undefined" );
	    }

	    child = XML::get_next_nontext( child->next );
	    // Parse DataInputs and make a map for each input string -> xmlNode*
	    WPS::Service::ParameterMap input_parameter_map;
	    if ( child && !xmlStrcmp(child->name, (const xmlChar*)"DataInputs") )
	    {
		xmlNode* node = XML::get_next_nontext( child->children );
		while ( node )
		{
		    if ( xmlStrcmp(node->name, (const xmlChar*)"Input") )
		    {
			return print_exception( WPS_INVALID_PARAMETER_VALUE, "Only Input elements are allowed inside DataInputs" );
		    }
		    xmlNode* nnode = XML::get_next_nontext( node->children );
		    string identifier;
		    xmlNode* data = 0;
		    while ( nnode )
		    {
			if ( !xmlStrcmp( nnode->name, (const xmlChar*)"Identifier") )
			{
			    if ( nnode->children && nnode->children->content )
				identifier = (const char*)nnode->children->content;
			}
			else if (!xmlStrcmp( nnode->name, (const xmlChar*)"Data") )
			{
			    xmlNode *n = XML::get_next_nontext( nnode->children );
			    if ( n )
				data = nnode;
			}
			nnode = XML::get_next_nontext( nnode->next );
		    }

		    if ( identifier == "" )
		    {
			return print_exception( WPS_INVALID_PARAMETER_VALUE, "Input identifier undefined" );
		    }
		    if ( data == 0 )
		    {
			return print_exception( WPS_INVALID_PARAMETER_VALUE, "Undefined data" );
		    }

		    xmlNode * n = XML::get_next_nontext( data->children );
		    if ( xmlStrcmp(n->name, (const xmlChar*)"LiteralData") && xmlStrcmp(n->name, (const xmlChar*)"ComplexData") )
		    {
			return print_exception( WPS_INVALID_PARAMETER_VALUE, "Data must be either LiteralData or ComplexData" );
		    }
		    xmlNode *actual_data = XML::get_next_nontext( n->children );
		    // associate xml data to identifier
		    input_parameter_map[identifier] = actual_data;

		    // next input
		    node = XML::get_next_nontext( node->next );
		}
	    }
	    else
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "DataInputs undefined" );
	    }

	    child = XML::get_next_nontext( child->next );
	    if ( child && !xmlStrcmp(child->name, (const xmlChar*)"ResponseForm") )
	    {
		child = XML::get_next_nontext( child->children );
		if ( child && xmlStrcmp( child->name, (const xmlChar*)"RawDataOutput") )
		{
		    return print_exception( WPS_INVALID_PARAMETER_VALUE, "Only raw data output are supported" );
		}
	    }
	    else
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "Responseform undefined" );
	    }

	    if ( !WPS::Service::exists(identifier) )
	    {
		return print_exception( WPS_INVALID_PARAMETER_VALUE, "Unknown service identifier " + identifier );
	    }
	    // all inputs are now defined, parse them
        std::auto_ptr<WPS::Service> service( WPS::Service::get_service( identifier )->clone() );
	    try
	    {
		WPS::Service::ParameterMap& output_parameters = service->execute( input_parameter_map );
		
		outs_ << "Content-type: text/xml" << endl;
		outs_ << endl;
		service->get_xml_execute_response( outs_, getParam("REQUEST_URI") );
	    }
	    catch (std::invalid_argument& e)
	    {
	     	return print_exception( WPS_INVALID_PARAMETER_VALUE, e.what() );
	    }
	    catch (std::runtime_error& e)
	    {
		COUT << "Runtime error " << e.what() << endl;
		return print_error_status( 400, e.what() );
	    }
	}
	else
	{
	    return print_exception( WPS_OPERATION_NOT_SUPPORTED, "Request is for an operation that is not supported by this server" );
	}
    
	return 0;
    }
};
