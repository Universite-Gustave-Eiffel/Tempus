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

#ifndef TEMPUS_VARIANT_HH
#define TEMPUS_VARIANT_HH

#include <map>
#include <string>
#include <boost/variant.hpp>

namespace Tempus {

///
/// Variant type
enum VariantType {
    BoolVariant,
    IntVariant,
    FloatVariant, // stored as a double
    StringVariant
};

///
/// class Variant
/// Used to store plugin option values and metric values
/// FIXME: why not use boost::variant ?
class Variant {
public:
    Variant();
    explicit Variant( bool b );
    explicit Variant( int64_t i);
    explicit Variant( double f );
    explicit Variant( const std::string& s );

    // return a string representation
    std::string str() const;
    VariantType type() const;

    static Variant from_bool( bool b );
    static Variant from_int( int64_t i );
    static Variant from_float( double f );
    static Variant from_string( const std::string& s, VariantType = StringVariant );

    template <typename T>
    T as() const {
        T v;
        convert_to( v );
        return v;
    }
private:
    typedef boost::variant<bool, int64_t, double, std::string> ValueT;
    ValueT v_;

    void convert_to( bool& b ) const;
    void convert_to( int64_t& i ) const;
    void convert_to( double& f ) const;
    void convert_to( std::string& s ) const;
};

typedef std::map<std::string, Variant> VariantMap;

} // Tempus

#endif
