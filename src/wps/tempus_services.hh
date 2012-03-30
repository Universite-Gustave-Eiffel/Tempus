// WPS Tempus services
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence
/**
   Tempus WPS services: implementation of abstract WPS services
 */

#ifndef TEMPUS_WPS_SERVICES_HH
#define TEMPUS_WPS_SERVICES_HH

#include "../core/request.hh"
#include "wps_service.hh"

namespace WPS
{

    class ServiceWithRequest : public Service, public Tempus::Request
    {
    public:
	///
	/// Constructor
	ServiceWithRequest( const std::string& name );

	virtual void parse_xml_parameters( InputParameterMap& input_parameter_map );
    };

}; // WPS namespace

#endif
