// WPS Tempus services
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence
/**
   Tempus WPS services: implementation of abstract WPS services
 */

#ifndef TEMPUS_WPS_SERVICES_HH
#define TEMPUS_WPS_SERVICES_HH

#include <string>

#include "request.hh"
#include "wps_service.hh"

namespace WPS
{
    class PreBuildService : public Service
    {
    public:
	PreBuildService();

	virtual void parse_xml_parameters( InputParameterMap& input_parameter_map );
	virtual void execute();
    protected:
	std::string db_options_;
    };

    class BuildService : public Service
    {
    public:
	BuildService();

	virtual void execute();
    };

    class PreProcessService : public Service, public Tempus::Request
    {
    public:
	///
	/// Constructor
	PreProcessService();

	virtual void parse_xml_parameters( InputParameterMap& input_parameter_map );
	virtual void execute();
    };

}; // WPS namespace

#endif
