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

#include "cast.hh"

namespace {
template <typename T>
struct VariantCast {
    T operator()( const std::string& str ) {
        return Tempus::lexical_cast<T>( str );
    }
};
template <>
struct VariantCast<bool> {
    bool operator()( const std::string& str ) {
        return str == "true";
    }
};
}


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
    Variant( bool b );
    Variant( size_t i = 0 );
    Variant( double f );
    Variant( const std::string& s, VariantType = StringVariant );
    
    std::string str() const {
        return str_;
    }
    VariantType type() const {
        return type_;
    }

    template <typename T>
    T as() const {
        return VariantCast<T>()( str_ );
    }
private:
    VariantType type_;
    // string representation of the value
    std::string str_;
};

typedef std::map<std::string, Variant> VariantMap;

} // Tempus

#endif
