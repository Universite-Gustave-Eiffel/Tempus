// WPS service
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence
/**
   A WPS Service is a generic process callable through the 'Execute' WPS operation.
 */

#ifndef TEMPUS_WPS_SERVER_HH
#define TEMPUS_WPS_SERVER_HH

#include <map>
#include <string>

#include "plugin.hh"
#include "xml_helper.hh"

namespace WPS
{
    ///
    /// Function callable from a WPS 'Execute' operation
    class Service
    {
    public:
	typedef std::map<std::string, xmlNode*> InputParameterMap;

	Service( const std::string& name ) : name_(name)
	{
	    // It is never freed. It is ok since it is a static object
	    if ( !services_ )
		services_ = new std::map<std::string, Service*>();
	    // Add this service to the global map of services
	    (*services_)[name] = this;
	}
	
	///
	/// Check input parameters against their XML schemas
	virtual void check_parameters( InputParameterMap& input_parameter_map );

	///
	/// Extract input parameters
	virtual void parse_xml_parameters( InputParameterMap& input_parameter_map )
	{
	    check_parameters( input_parameter_map );
	}

	virtual void execute() = 0;

	// FIXME : return in raw XML
	///
	/// Returns an XML string that conforms to a DescribeProcess operation
	std::ostream& get_xml_description( std::ostream& out );

	///
	/// Global service map interface: returns a Service* based on a service name
	static Service* get_service( const std::string& name );

	///
	/// Global service map interface: tests if a service exists
	static bool exists( const std::string& name )
	{
	    return services_->find( name ) != services_->end();
	}

	static void set_plugin( Tempus::Plugin* plugin )
	{
	    plugin_ = plugin;
	}

	///
	/// Global service map interface: returns an XML string that conforms to a 'GetCapabilities' operation
	static std::ostream& get_xml_capabilities( std::ostream& out );

    protected:
	///
	/// A global map of services
	static std::map<std::string, Service*> *services_;
	///
	/// The current Tempus plugin accessed
	static Tempus::Plugin* plugin_;
	
	struct ParameterSchema
	{
	    std::string schema;
	    // complexType or not ?
	    bool is_complex;
	};
	std::map<std::string, ParameterSchema> input_parameter_schema_;
	std::string name_;

	///
	/// Adds an input parameter definition. To be called by derived classes in their constructor
	void add_input_parameter( const std::string& name, const std::string& schema, bool is_complex = true )
	{
	    input_parameter_schema_[name].schema = schema;
	    input_parameter_schema_[name].is_complex = is_complex;
	}
    };

}; // WPS namespace

#endif
