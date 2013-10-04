// WPS service
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence

#ifndef TEMPUS_WPS_SERVER_HH
#define TEMPUS_WPS_SERVER_HH

/**
   A WPS Service is a generic process callable through the 'Execute' WPS operation.
 */

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
	typedef std::map<std::string, xmlNode*> ParameterMap;

	Service( const std::string& name ) : name_(name)
	{
	    // It is never freed. It is ok since it is a static object
	    if ( !services_ )
		services_ = new std::map<std::string, Service*>();
	    // Add this service to the global map of services
	    (*services_)[name] = this;
	}
	
	///
	/// Extract input parameters
	void parse_xml_parameters( ParameterMap& input_parameter_map );

	virtual ParameterMap& execute( ParameterMap& input_parameter_map )
	{
	    parse_xml_parameters( input_parameter_map );
	    // No output by default
	    output_parameters_.clear();
	    return output_parameters_;
	}

    virtual Service * clone() const { throw std::runtime_error("not implemented");}

	///
	/// Returns an XML string that conforms to a DescribeProcess operation
	std::ostream& get_xml_description( std::ostream& out );

	///
	/// Returns an XML string that represents results of an Execute operation
	std::ostream& get_xml_execute_response( std::ostream& out, const std::string & service_instance );

	///
	/// Global service map interface: returns a Service* based on a service name
	static Service* get_service( const std::string& name );

	///
	/// Global service map interface: tests if a service exists
	static bool exists( const std::string& name )
	{
	    return services_->find( name ) != services_->end();
	}

	///
	/// Global service map interface: returns an XML string that conforms to a 'GetCapabilities' operation
	static std::ostream& get_xml_capabilities( std::ostream& out, const std::string & script_name );

    protected:
	///
	/// A global map of services
	static std::map<std::string, Service*> *services_;
	
	struct ParameterSchema
	{
	    std::string schema;
	    // complexType or not ?
	    bool is_complex;
	};
	typedef std::map<std::string, ParameterSchema> SchemaMap;
	SchemaMap input_parameter_schema_;
	SchemaMap output_parameter_schema_;
	std::string name_;

	///
	/// Check parameters against their XML schemas
	virtual void check_parameters( ParameterMap& parameter_map, SchemaMap& schema_map );

	///
	/// Adds an input parameter definition. To be called by derived classes in their constructor
	void add_input_parameter( const std::string& name, const std::string& schema, bool is_complex = true )
	{
	    input_parameter_schema_[name].schema = schema;
	    input_parameter_schema_[name].is_complex = is_complex;
	}

	///
	/// Adds an output parameter definition. To be called by derived classes in their constructor
	void add_output_parameter( const std::string& name, const std::string& schema, bool is_complex = true )
	{
	    output_parameter_schema_[name].schema = schema;
	    output_parameter_schema_[name].is_complex = is_complex;
	}

	///
	/// Output parameters
	ParameterMap output_parameters_;
    };

}; // WPS namespace

#endif
