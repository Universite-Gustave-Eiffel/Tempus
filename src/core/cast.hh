/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
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

#ifndef TEMPUS_CAST_HH
#define TEMPUS_CAST_HH

#include <string>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <iomanip>

namespace Tempus
{

// because boost::lexical_cast calls locale, which is not thread safe
struct bad_lexical_cast : public std::exception {
    bad_lexical_cast( const std::string& msg ) : std::exception(), msg_( msg ) {}
    virtual const char* what() const throw() {
        return msg_.c_str();
    }
    virtual ~bad_lexical_cast() throw() {}
    std::string msg_;
};
template <typename TOUT>
struct lexical_cast_aux_ {
    TOUT operator()( const std::string& in ) {
        TOUT out;

        if( !( std::istringstream( in ) >> out ) ) {
            throw bad_lexical_cast( "cannot cast " + in + " to " + typeid( TOUT ).name() );
        }

        return out;
    }
};
template <>
struct lexical_cast_aux_<std::string> {
    std::string operator()( const std::string& in ) {
        return in;
    }
};

template <typename TOUT>
TOUT lexical_cast( const std::string& in )
{
    return lexical_cast_aux_<TOUT>()( in );
}

template <typename TOUT>
TOUT lexical_cast( const unsigned char* in ) // for xmlChar*
{
    return lexical_cast<TOUT>( std::string( reinterpret_cast<const char*>( in ) ) );
}

template <typename TIN>
std::string to_string( const TIN& in )
{
    std::stringstream ss;
    ss << in;
    return ss.str();
}

template <>
inline std::string to_string<float>( const float& f )
{
    std::ostringstream ss;
    ss << std::setprecision(9) << f;
    return ss.str();
}

}

#endif
