// WPS request
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence

#ifndef TEMPUS_WPS_REQUEST_HH
#define TEMPUS_WPS_REQUEST_HH

#include <iostream>
#include <string>

#define WPS_INVALID_PARAMETER_VALUE "InvalidParameterValue"
#define WPS_OPERATION_NOT_SUPPORTED "OperationNotSupported"

namespace WPS
{
    ///
    /// WPS::Request
    class Request
    {
    public:
	Request( std::streambuf* ins, std::streambuf* outs ) : ins_(ins), outs_(outs) {}

	int process();

	int print_error_status( int status, const std::string& msg );
	int print_exception( const std::string& type, const std::string& msg );
    protected:
	std::istream ins_;
	std::ostream outs_;
    };
};

#endif
