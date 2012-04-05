#include <iostream>
#include <stdexcept>

#include "wps_service.hh"

using namespace std;

namespace WPS
{
    std::map<std::string, Service*>* Service::services_ = 0;
    Tempus::Plugin* Service::plugin_ = 0;
    
    void Service::check_input_parameters( ParameterMap& input_parameter_map )
    {
	cout << "mapsize = " << input_parameter_map.size() << " schemasize = " << input_parameter_schema_.size() << endl;
	if ( input_parameter_map.size() != input_parameter_schema_.size() )
	{
	    throw std::invalid_argument( "Wrong number of inputs" );
	}
	ParameterMap::iterator it;
	for ( it = input_parameter_map.begin(); it != input_parameter_map.end(); it++ )
	{
	    if ( input_parameter_schema_.find( it->first ) == input_parameter_schema_.end() )
	    {
		throw std::invalid_argument( "Unknown input parameter " + it->first );
	    }
	    
	    XML::ensure_validity( it->second, input_parameter_schema_[it->first].schema );
	}
    }

    std::ostream& Service::get_xml_description( std::ostream& out )
    {
	out << "<wps:ProcessDescriptions xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:wps=\"http://www.opengis.net/wps/1.0.0\" xmlns:ows=\"http://www.opengis.net/ows/1.1\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd\" service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-US\">\n";
	out << "<ProcessDescription wps:processVersion=\"1.0\" storeSupported=\"false\" statusSupported=\"false\">\n";
	out << "<ows:Identifier>" << name_ << "</ows:Identifier>\n";
	out << "<ows:Title>" << name_ << "</ows:Title>\n";
	out << "<ows:Abstract>" << name_ << "</ows:Abstract>\n";
	
	out << "<DataInputs>" << endl;
	std::map<std::string, ParameterSchema>::iterator it;
	for ( it = input_parameter_schema_.begin(); it != input_parameter_schema_.end(); it++ )
	{
	    out << "<Input><ows:Identifier>" << it->first << "</ows:Identifier>";
	    out << "<ows:Title>" << it->first << "</ows:Title>";
	    if (it->second.is_complex)
	    {
		out << "<ComplexData>";
		out << "<Default><Format><MimeType>text/xml</MimeType><Encoding>UTF-8</Encoding><Schema>" + it->second.schema;
		out << "</Schema></Format></Default>";
		out << "<Supported><Format><MimeType>text/xml</MimeType><Encoding>UTF-8</Encoding><Schema>" + it->second.schema;
		out << "</Schema></Format></Supported>";
		out << "</ComplexData>" << endl;
	    }
	    else
	    {
		// FIXME !
		out << "<LiteralData>" + it->second.schema << "</LiteralData>" << endl;
	    }
	    out << "</Input>" << endl;
	}
	out << "</DataInputs>" << endl;
	
	out << "<ProcessOutputs>" << endl;
	for ( it = output_parameter_schema_.begin(); it != output_parameter_schema_.end(); it++ )
	{
	    out << "<Output><ows:Identifier>" << it->first << "</ows:Identifier>";
	    out << "<ows:Title>" << it->first << "</ows:Title>";
	    if (it->second.is_complex)
	    {
		out << "<ComplexData>";
		out << "<Default><Format><MimeType>text/xml</MimeType><Encoding>UTF-8</Encoding><Schema>" + it->second.schema;
		out << "</Schema></Format></Default>";
		out << "<Supported><Format><MimeType>text/xml</MimeType><Encoding>UTF-8</Encoding><Schema>" + it->second.schema;
		out << "</Schema></Format></Supported>";
		out << "</ComplexData>" << endl;
	    }
	    else
	    {
		// FIXME !
		out << "<LiteralData>" + it->second.schema << "</LiteralData>" << endl;
	    }
	    out << "</Output>" << endl;
	}
	out << "</ProcessOutputs>" << endl;
	out << "</ProcessDescription>" << endl;
	out << "</wps:ProcessDescriptions>" << endl;
	
	return out;
    }

    Service* Service::get_service( const std::string& name )
    {
	if ( exists(name) )
	    return (*services_)[name];
	return 0;
    }

    std::ostream& Service::get_xml_capabilities( std::ostream& out )
    {
	string script_name = getenv( "SCRIPT_NAME" );
	
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

};
