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
    class StateService : public Service
    {
    public:
	StateService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

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

    class PluginListService : public Service
    {
    public:
	PluginListService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    ///
    /// Base class used for services of a plugin.
    /// Input parameter: a plugin identifier
    class PluginService : public Service
    {
    public:
	PluginService( const std::string& name );
	Tempus::Plugin* get_plugin( ParameterMap& input_parameters );
    };

    class GetOptionsDescService : public PluginService
    {
    public:
	///
	/// Constructor
	GetOptionsDescService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class GetOptionsService : public PluginService
    {
    public:
	///
	/// Constructor
	GetOptionsService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class SetOptionsService : public PluginService
    {
    public:
	///
	/// Constructor
	SetOptionsService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class GetMetricsService : public PluginService
    {
    public:
	///
	/// Constructor
	GetMetricsService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class PreProcessService : public PluginService, public Tempus::Request
    {
    public:
	///
	/// Constructor
	PreProcessService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class ProcessService : public PluginService
    {
    public:
	///
	/// Constructor
	ProcessService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

    class ResultService : public PluginService
    {
    public:
	///
	/// Constructor
	ResultService();
	virtual ParameterMap& execute( ParameterMap& input_parameter_map );
    };

}; // WPS namespace

#endif
