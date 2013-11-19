/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *   
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

// WPS request

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
