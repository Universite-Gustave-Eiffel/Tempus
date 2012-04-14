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
    class ConnectService : public Service
    {
    public:
	ConnectService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class PreBuildService : public Service
    {
    public:
	PreBuildService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class BuildService : public Service
    {
    public:
	BuildService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class LoadPluginService : public Service
    {
    public:
	LoadPluginService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class UnloadPluginService : public Service
    {
    public:
	UnloadPluginService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class PreProcessService : public Service, public Tempus::Request
    {
    public:
	///
	/// Constructor
	PreProcessService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class ProcessService : public Service
    {
    public:
	///
	/// Constructor
	ProcessService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class ResultService : public Service
    {
    public:
	///
	/// Constructor
	ResultService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

}; // WPS namespace

#endif
