#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

///
/// Template response to a GetCapabilities request
/// $WPS_URL is replaced by the actual url
const char get_capabilities_tmpl[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<wps:Capabilities service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-CA\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd\" updateSequence=\"1\">\n"
    "  <ows:ServiceIdentification>\n"
    "    <ows:Title>IFSTTAR Tempus WPS server</ows:Title>\n"
    "    <ows:ServiceType>WPS</ows:ServiceType>\n"
    "    <ows:ServiceTypeVersion>1.0.0</ows:ServiceTypeVersion>\n"
    "  </ows:ServiceIdentification>\n"
    "  <ows:ServiceProvider>\n"
    "    <ows:ProviderName>Oslandia</ows:ProviderName>\n"
    "  </ows:ServiceProvider>\n"
    "  <ows:OperationsMetadata>\n"
    "    <ows:Operation name=\"GetCapabilities\">\n"
    "      <ows:DCP>\n"
    "	<ows:HTTP>\n"
    "	  <ows:Get xlink:href=\"$WPS_URL\"/>\n"
    "	</ows:HTTP>\n"
    "      </ows:DCP>\n"
    "    </ows:Operation>\n"
    "    <ows:Operation name=\"DescribeProcess\">\n"
    "      <ows:DCP>\n"
    "	<ows:HTTP>\n"
    "	  <ows:Get xlink:href=\"$WPS_URL\"/>\n"
    "	</ows:HTTP>\n"
    "      </ows:DCP>\n"
    "    </ows:Operation>\n"
    "    <ows:Operation name=\"Execute\">\n"
    "      <ows:DCP>\n"
    "	<ows:HTTP>\n"
    "	  <ows:Get xlink:href=\"$WPS_URL\"/>\n"
    "	</ows:HTTP>\n"
    "      </ows:DCP>\n"
    "    </ows:Operation>\n"
    "  </ows:OperationsMetadata>\n"
    "\n"
    "  <wps:ProcessOfferings>\n"
    "    <wps:Process wps:processVersion=\"1.0\">\n"
    "      <ows:Identifier>build</ows:Identifier>\n"
    "      <ows:Title>Build graphs in memory</ows:Title>\n"
    "    </wps:Process>\n"
    "\n"
    "    <wps:Process wps:processVersion=\"1.0\">\n"
    "      <ows:Identifier>pre_process</ows:Identifier>\n"
    "      <ows:Title>Pre-process a user request</ows:Title>\n"
    "    </wps:Process>\n"
    "\n"
    "    <wps:Process wps:processVersion=\"1.0\">\n"
    "      <ows:Identifier>process</ows:Identifier>\n"
    "      <ows:Title>Process a user request</ows:Title>\n"
    "    </wps:Process>\n"
    "\n"
    "    <wps:Process wps:processVersion=\"1.0\">\n"
    "      <ows:Identifier>post_process</ows:Identifier>\n"
    "      <ows:Title>Post-Process a user request</ows:Title>\n"
    "    </wps:Process>\n"
    "\n"
    "    <wps:Process wps:processVersion=\"1.0\">\n"
    "      <ows:Identifier>result</ows:Identifier>\n"
    "      <ows:Title>Get a request's result</ows:Title>\n"
    "    </wps:Process>\n"
    "  </wps:ProcessOfferings>\n"
    "\n"
    "  <wps:Languages>\n"
    "    <wps:Default>\n"
    "      <ows:Language>en-US</ows:Language>\n"
    "    </wps:Default>\n"
    "    <wps:Supported>\n"
    "      <ows:Language>en-US</ows:Language>\n"
    "    </wps:Supported>\n"
    "  </wps:Languages> \n"
    "</wps:Capabilities>\n";


const char describe_process_tmpl[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<wps:ProcessDescriptions xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd\" service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-CA\">\n"
    "  <ProcessDescription wps:processVersion=\"1.0\" storeSupported=\"false\" statusSupported=\"false\">\n"
    "    <ows:Identifier>pre_process</ows:Identifier>\n"
    "    <ows:Title>Pre-process a user request</ows:Title>\n"
    "    <ows:Abstract>Pre-process a user request</ows:Abstract>\n"
    "    <DataInputs>\n"
    "      <Input minOccurs=\"1\" maxOccurs=\"1\">\n"
    "	<ows:Identifier>request</ows:Identifier>\n"
    "	<ows:Title>User request</ows:Title>\n"
    "	<ComplexData>\n"
    "	  <Default>\n"
    "	    <Format>\n"
    "	      <MimeType>text/xml</MimeType>\n"
    "	      <Encoding>UTF-8</Encoding>\n"
    "	      <Schema>\n"
    "		<element name=\"request\">\n"
    "		  <complexType>\n"
    "		    <sequence>\n"
    "		      <element name=\"origin\" type=\"xs:string\"/>\n"
    "		      <element name=\"parking_location\" type=\"xs:string\"/>\n"
    "		    </sequence>\n"
    "		  </complexType>\n"
    "		</element>\n"
    "	      </Schema>\n"
    "	    </Format>\n"
    "	  </Default>\n"
    "	  <Supported>\n"
    "	    <Format>\n"
    "	      <MimeType>text/xml</MimeType>\n"
    "	      <Encoding>UTF-8</Encoding>\n"
    "	      <Schema>\n"
    "		<element name=\"request\">\n"
    "		  <complexType>\n"
    "		    <sequence>\n"
    "		      <element name=\"origin\" type=\"xs:string\"/>\n"
    "		      <element name=\"parking_location\" type=\"xs:string\"/>\n"
    "		    </sequence>\n"
    "		  </complexType>\n"
    "		</element>\n"
    "	      </Schema>\n"
    "	    </Format>\n"
    "	  </Supported>\n"
    "	</ComplexData>\n"
    "      </Input>\n"
    "    </DataInputs>\n"
    "    <ProcessOutputs>\n"
    "      <!-- void -->\n"
    "    </ProcessOutputs>\n"
    "    \n"
    "  </ProcessDescription>\n"
    "</wps:ProcessDescriptions>\n";


struct XMLNode
{
    std::list<XMLNode*> children;
    std::map<std::string, std::string> attributes;
    std::string name;
    std::string content;

    virtual ~XMLNode()
    {
	// cascading delete
	for ( std::list<XMLNode*>::iterator it = children.begin(); it != children.end(); it++ )
	{
	    delete *it;
	}
    }
};

ostream& operator<<( ostream& ostr, XMLNode& node )
{
    if (( node.children.size() == 0 ) && ( node.content.size() == 0 ))
    {
	ostr << "<" << node.name << "/>" << endl;
    }
    else
    {
	ostr << "<" << node.name;
	for ( std::map<std::string, std::string>::iterator it = node.attributes.begin(); it != node.attributes.end(); it++ )
	{
	    ostr << " " << it->first << "=\"" << it->second << "\"";
	}
	ostr << ">" << endl;
	for ( std::list<XMLNode*>::const_iterator it = node.children.begin(); it != node.children.end(); it++ )
	{
	    ostr << *(*it) << endl;
	}
	ostr << node.content << endl;
	ostr << "</" << node.name << ">" << endl;
    }
    return ostr;
}


void return_error_status( int status, std::string msg )
{
    cout << "Status: " + boost::lexical_cast<string>(status) + " " + msg << endl;
    cout << "Content-Type: text/html" << endl;
    cout << endl;
    cout << "<h2>" + msg + "</h2>" << endl;
    exit( status );
}

//TODO : return exception

int main()
{
    string request_method = getenv( "REQUEST_METHOD" );
    string query_string = getenv( "QUERY_STRING" );

    if ( request_method != "GET" )
    {
	return_error_status( 405, "Method not allowed" );
    }

    //
    // parse query
    map<string, string> query;
    vector<string> strs;
    boost::split( strs, query_string, boost::is_any_of("&") );
    for ( size_t i = 0; i < strs.size(); i++ )
    {
	size_t idx = strs[i].find('=');
	if ( idx == string::npos )
	{
	    query[strs[i]] = "";
	}
	else
	{
	    string key = strs[i].substr(0, idx);
	    query[key] = strs[i].substr( idx+1 );
	}
    }

    if ( boost::to_lower_copy(query["service"]) != "wps" )
    {
	return_error_status( 400, "Only 'wps' service is supported." );
    }
    if ( query["version"] != "1.0.0" )
    {
	return_error_status( 400, "Only '1.0.0' version is supported" );
    }

    string wps_request = query["request"];
    if (( wps_request != "GetCapabilities" ) && ( wps_request != "DescribeProcess" ) && ( wps_request != "Execute" ))
    {
	return_error_status( 400, "WPS request not implemented");
    }

    if ( wps_request == "GetCapabilities" )
    {
	cout << "Content-Type: text/xml" << endl;
	cout << endl;

	string pattern = "$WPS_URL";
	string script_name = getenv( "SCRIPT_NAME" );

	cout << boost::replace_all_copy( string(get_capabilities_tmpl), pattern, script_name );
	cout.flush();
    }
    else if ( wps_request == "DescribeProcess" )
    {
	cout << "Content-Type: text/xml" << endl;
	cout << endl;

	cout << describe_process_tmpl;
	cout.flush();
	// TODO
    }
    else if ( wps_request == "Execute" )
    {
	// TODO
    }

    cerr << getenv("REQUEST_URI") << endl;

    return 404;
}
