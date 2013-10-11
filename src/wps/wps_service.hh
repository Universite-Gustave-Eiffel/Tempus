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

	Service( const std::string& name );
        virtual ~Service();
	
	///
	/// Extract input parameters
	void parse_xml_parameters( const ParameterMap& input_parameter_map ) const;

	virtual ParameterMap execute( const ParameterMap& input_parameter_map ) const;

	///
	/// Returns an XML string that conforms to a DescribeProcess operation
	std::ostream& get_xml_description( std::ostream& out ) const;

	///
	/// Returns an XML string that represents results of an Execute operation
	std::ostream& get_xml_execute_response( std::ostream& out, const std::string & service_instance, const ParameterMap& outputs ) const;

	///
	/// Global service map interface: returns a Service* based on a service name.
        /// The returned service is const and can be used in different threads
	static const Service* get_service( const std::string& name );

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
	typedef std::map<std::string, XML::Schema*> SchemaMap;
	SchemaMap input_parameter_schema_;
	SchemaMap output_parameter_schema_;
	std::string name_;

	///
	/// Check parameters against their XML schemas
	virtual void check_parameters( const ParameterMap& parameter_map, const SchemaMap& schema_map ) const;

	///
	/// Adds an input parameter definition. To be called by derived classes in their constructor
        /// @param[in] name Name of the parameter
        /// @param[in] schema The schema filename. If empty, a schema will be looked for
        ///                   at DATA_DIR/wps_schemas/service_name/parameter_name.xsd
	void add_input_parameter( const std::string& name, const std::string& schema = "" );

	///
	/// Adds an output parameter definition. To be called by derived classes in their constructor
        /// @param[in] schema The schema filename. If empty, a schema will be looked for
        ///                   at DATA_DIR/wps_schemas/service_name/parameter_name.xsd
	void add_output_parameter( const std::string& name, const std::string& schema = "" );
    };

} // WPS namespace

#endif
