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

	virtual void parse_xml_parameters( ParameterMap& input_parameter_map );
	virtual ParameterMap& execute();

	static std::string db_options;
    };

    class BuildService : public Service
    {
    public:
	BuildService();

	virtual ParameterMap& execute();
    };

    class PreProcessService : public Service, public Tempus::Request
    {
    public:
	///
	/// Constructor
	PreProcessService();

	virtual void parse_xml_parameters( ParameterMap& input_parameter_map );
	virtual ParameterMap& execute();
    };

    class ProcessService : public Service
    {
    public:
	///
	/// Constructor
	ProcessService();

	virtual ParameterMap& execute();
    };

    class ResultService : public Service
    {
    public:
	///
	/// Constructor
	ResultService();

	virtual ParameterMap& execute();
    };

}; // WPS namespace

#endif
