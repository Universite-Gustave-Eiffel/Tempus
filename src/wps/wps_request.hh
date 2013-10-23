// WPS request
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence

#ifndef TEMPUS_WPS_REQUEST_HH
#define TEMPUS_WPS_REQUEST_HH

#include <iostream>
#include <string>
#include <fcgi_stdio.h>

#define WPS_INVALID_PARAMETER_VALUE "InvalidParameterValue"
#define WPS_OPERATION_NOT_SUPPORTED "OperationNotSupported"
#define WPS_NO_APPLICABLE_CODE      "NoApplicableCode"

namespace WPS
{
    ///
    /// WPS::Request. It is in charge of processing a WPS request
    class Request
    {
    public:
	Request( std::streambuf* ins, std::streambuf* outs, char** env ) : ins_(ins), outs_(outs), env_(env) 
    {}

	int process();
    const char * getParam( const char * name )
    { 
        return FCGX_GetParam( name, env_ ); 
    }

	int print_error_status( int status, const std::string& msg );
	int print_exception( const std::string& type, const std::string& msg );
    protected:
	std::istream ins_;
	std::ostream outs_;
        char ** env_;
    };
}

#endif
