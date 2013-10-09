#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include "wps_service.hh"

using namespace std;

namespace WPS
{
    std::map<std::string, Service*>* Service::services_ = 0;
    
    void Service::parse_xml_parameters( ParameterMap& input_parameter_map )
    {
	// default behaviour: only check schemas
	check_parameters( input_parameter_map, input_parameter_schema_ );
    }

    void Service::check_parameters( ParameterMap& parameter_map, SchemaMap& schema_map )
    {
	if ( parameter_map.size() != schema_map.size() )
	{
            std::string argList;
            ParameterMap::const_iterator it;
            for ( it = parameter_map.begin(); it != parameter_map.end(); it++ )
            {
                argList += " " + it->first;
            }
            
	    throw std::invalid_argument( (boost::format("Wrong number of parameters got %1% arguments (%3%), %2% expected")
                                          % parameter_map.size()
                                          % schema_map.size()
                                          % argList).str() );
	}
	ParameterMap::iterator it;
	for ( it = parameter_map.begin(); it != parameter_map.end(); it++ )
	{
	    if ( schema_map.find( it->first ) == schema_map.end() )
	    {
		throw std::invalid_argument( "Unknown parameter " + it->first );
	    }
	    
	    XML::ensure_validity( it->second, schema_map[it->first].schema );
	}
    }

    std::ostream& Service::get_xml_description( std::ostream& out )
    {
	out << "<wps:ProcessDescriptions xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd\" service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-US\">\n";
	out << "  <ProcessDescription wps:processVersion=\"1.0\" storeSupported=\"false\" statusSupported=\"false\">" << endl;
	out << "    <ows:Identifier>" << name_ << "</ows:Identifier>" << endl;
	out << "    <ows:Title>" << name_ << "</ows:Title>" << endl;
	out << "    <ows:Abstract>" << name_ << "</ows:Abstract>" << endl;
	
	out << "    <DataInputs>" << endl;
	std::map<std::string, ParameterSchema>::iterator it;
	for ( it = input_parameter_schema_.begin(); it != input_parameter_schema_.end(); it++ )
	{
	    out << "      <Input>" << endl;
	    out << "        <ows:Identifier>" << it->first << "</ows:Identifier>" << endl;
	    out << "        <ows:Title>" << it->first << "</ows:Title>" << endl;
	    if (it->second.is_complex)
	    {
		out << "        <ComplexData>" << endl;
		out << "          <Default>" << endl;
		out << "            <Format>" << endl;
		out << "              <MimeType>text/xml</MimeType>" << endl;
		out << "              <Encoding>UTF-8</Encoding>" << endl;
		out << "              <Schema>" << it->second.schema << "</Schema>" << endl;
		out << "            </Format>" << endl;
		out << "          </Default>" << endl;
		out << "          <Supported>" << endl;
		out << "            <Format>" << endl;
		out << "              <MimeType>text/xml</MimeType>" << endl;
		out << "              <Encoding>UTF-8</Encoding>" << endl;
		out << "              <Schema>" << it->second.schema << "</Schema>" << endl;
		out << "            </Format>" << endl;
		out << "          </Supported>" << endl;
		out << "        </ComplexData>" << endl;
	    }
	    else
	    {
		// FIXME !
		out << "<LiteralData>" + it->second.schema << "</LiteralData>" << endl;
	    }
	    out << "      </Input>" << endl;
	}
	out << "    </DataInputs>" << endl;
	
	out << "    <ProcessOutputs>" << endl;
	for ( it = output_parameter_schema_.begin(); it != output_parameter_schema_.end(); it++ )
	{
	    out << "      <Output>" << endl;
	    out << "        <ows:Identifier>" << it->first << "</ows:Identifier>" << endl;
	    out << "        <ows:Title>" << it->first << "</ows:Title>" << endl;
	    if (it->second.is_complex)
	    {
		out << "        <ComplexData>" << endl;
		out << "          <Default>" << endl;
		out << "            <Format>" << endl;
		out << "              <MimeType>text/xml</MimeType>" << endl;
		out << "              <Encoding>UTF-8</Encoding>" << endl;
		out << "              <Schema>" << it->second.schema << "</Schema>" << endl;
		out << "            </Format>" << endl;
		out << "          </Default>" << endl;
		out << "          <Supported>" << endl;
		out << "            <Format>" << endl;
		out << "              <MimeType>text/xml</MimeType>" << endl;
		out << "              <Encoding>UTF-8</Encoding>" << endl;
		out << "              <Schema>" << it->second.schema << "</Schema>" << endl;
		out << "            </Format>" << endl;
		out << "          </Supported>" << endl;
		out << "        </ComplexData>" << endl;
	    }
	    else
	    {
		// FIXME !
		out << "<LiteralData>" + it->second.schema << "</LiteralData>" << endl;
	    }
	    out << "      </Output>" << endl;
	}
	out << "    </ProcessOutputs>" << endl;
	out << "  </ProcessDescription>" << endl;
	out << "</wps:ProcessDescriptions>" << endl;
	
	return out;
    }

    std::ostream& Service::get_xml_execute_response( std::ostream& out, const std::string & service_instance )
    {
	check_parameters( output_parameters_, output_parameter_schema_ );

	out << "<wps:ExecuteResponse xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd\" service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-US\" serviceInstance=\"" << service_instance << "\">" << endl;
	out << "  <wps:Process wps:processVersion=\"1\">" << endl;
	out << "    <ows:Identifier>" << name_ << "</ows:Identifier>" << endl;
	out << "    <ows:Title>" << name_ << "</ows:Title>" << endl;
	out << "  </wps:Process>" << endl;
	
	char now[21];
	time_t now_t;
	time( &now_t );
	size_t n = strftime( now, 20, "%Y-%m-%dT%H:%M:%S", localtime(&now_t) );
	now[n] = 0;
	out << "  <wps:Status creationTime=\"" << now << "\">" << endl;
	out << "    <wps:ProcessSucceeded/>" << endl;
	out << "  </wps:Status>" << endl;
	out << "  <wps:ProcessOutputs>" << endl;
	for ( ParameterMap::iterator it = output_parameters_.begin(); it != output_parameters_.end(); it++ )
	{
	    out << "    <wps:Output>" << endl;
	    out << "      <ows:Identifier>" << it->first << "</ows:Identifier>" << endl;
	    out << "      <ows:Title>" << it->first << "</ows:Title>" << endl;
	    out << "      <wps:Data>" << endl;
	    out << "        <wps:ComplexData>" << endl;
	    out << "          " << XML::to_string( it->second, 5 ) << endl;
	    // free xmlNode
	    xmlFreeNode( it->second );
	    out << "        </wps:ComplexData>" << endl;
	    out << "      </wps:Data>" << endl;
	    out << "    </wps:Output>" << endl;
	}
	out << "  </wps:ProcessOutputs>" << endl;
	out << "</wps:ExecuteResponse>" << endl;
	return out;
    }

    Service* Service::get_service( const std::string& name )
    {
	if ( exists(name) )
	    return (*services_)[name];
	return 0;
    }

    std::ostream& Service::get_xml_capabilities( std::ostream& out, const std::string & script_name )
    {
	
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<wps:Capabilities service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-US\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd\" updateSequence=\"1\">\n";
	out << "  <ows:ServiceIdentification>\n";
	out << "    <ows:Title>IFSTTAR Tempus WPS server</ows:Title>\n";
	out << "    <ows:ServiceType>WPS</ows:ServiceType>\n";
	out << "    <ows:ServiceTypeVersion>1.0.0</ows:ServiceTypeVersion>\n";
	out << "  </ows:ServiceIdentification>\n";
	out << "  <ows:ServiceProvider>\n";
	out << "    <ows:ProviderName>Oslandia</ows:ProviderName>\n";
	out << "  </ows:ServiceProvider>\n";
	out << "  <ows:OperationsMetadata>\n";
	out << "    <ows:Operation name=\"GetCapabilities\">\n";
	out << "      <ows:DCP>\n";
	out << "	<ows:HTTP>\n";
	out << "	  <ows:Get xlink:href=\"" << script_name << "\"/>\n";
	out << "	</ows:HTTP>\n";
	out << "      </ows:DCP>\n";
	out << "    </ows:Operation>\n";
	out << "    <ows:Operation name=\"DescribeProcess\">\n";
	out << "      <ows:DCP>\n";
	out << "	<ows:HTTP>\n";
	out << "	  <ows:Get xlink:href=\"" << script_name << "\"/>\n";
	out << "	</ows:HTTP>\n";
	out << "      </ows:DCP>\n";
	out << "    </ows:Operation>\n";
	out << "    <ows:Operation name=\"Execute\">\n";
	out << "      <ows:DCP>\n";
	out << "	<ows:HTTP>\n";
	out << "	  <ows:Post xlink:href=\"" << script_name << "\"/>\n";
	out << "	</ows:HTTP>\n";
	out << "      </ows:DCP>\n";
	out << "    </ows:Operation>\n";
	out << "  </ows:OperationsMetadata>\n";
	out << "\n";
	out << "  <wps:ProcessOfferings>\n";
	std::map<std::string, Service*>::iterator it;
	for ( it = services_->begin(); it != services_->end(); it++ )
	{
	    out << "    <wps:Process wps:processVersion=\"1.0\">\n";
	    out << "      <ows:Identifier>" << it->first << "</ows:Identifier>\n";
	    out << "      <ows:Title>" << it->first << "</ows:Title>\n";
	    out << "    </wps:Process>\n";
	}
	out << "  </wps:ProcessOfferings>\n";
	out << "\n";
	out << "  <wps:Languages>\n";
	out << "    <wps:Default>\n";
	out << "      <ows:Language>en-US</ows:Language>\n";
	out << "    </wps:Default>\n";
	out << "    <wps:Supported>\n";
	out << "      <ows:Language>en-US</ows:Language>\n";
	out << "    </wps:Supported>\n";
	out << "  </wps:Languages> \n";
	out << "</wps:Capabilities>\n";;
	return out;
    }

}
