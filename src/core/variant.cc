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

#include "variant.hh"
#include "cast.hh"

namespace Tempus {

Variant::Variant()
{
}

Variant::Variant( bool b )
{
    v_ = b;
}
Variant::Variant( int64_t i )
{
    v_ = i;
}
Variant::Variant( double i )
{
    v_ = i;
}

Variant::Variant( const std::string& s )
{
    v_ = s;
}

Variant Variant::from_bool( bool b )
{
    return Variant(b);
}

Variant Variant::from_int( int64_t i )
{
    return Variant(i);
}

Variant Variant::from_float( double f )
{
    return Variant(f);
}

Variant Variant::from_string( const std::string& s, VariantType t )
{
    switch (t)
    {
    case BoolVariant:
        return Variant(s=="true");
    case IntVariant:
        return Variant(Tempus::lexical_cast<int64_t>(s));
    case FloatVariant:
        return Variant(Tempus::lexical_cast<double>(s));
    case StringVariant:
        return Variant(s);
    }
    return Variant(s);
}

VariantType Variant::type() const
{
    // the order of the variant is the same as the VariantType enum
    return VariantType(v_.which());
}

std::string Variant::str() const
{
    if ( type() == StringVariant ) {
        return boost::get<std::string>( v_ );
    }
    if ( !v_.empty() ) {
        std::ostringstream ostr;
        ostr << v_;
        return ostr.str();
    }
    return std::string();
}

void Variant::convert_to( bool& b ) const
{
    if (type() == BoolVariant) {
        b = *boost::get<bool>(&v_);
    }
    else {
        b = str() == "true";
    }
}

void Variant::convert_to( int64_t& i ) const
{
    if (type() == IntVariant) {
        i = *boost::get<int64_t>(&v_);
    }
    else {
        i = Tempus::lexical_cast<int64_t>(str());
    }
}

void Variant::convert_to( double& f ) const
{
    if (type() == FloatVariant) {
        f = *boost::get<double>(&v_);
    }
    else {
        f = Tempus::lexical_cast<int64_t>(str());
    }
}

void Variant::convert_to( std::string& s ) const
{
    if (type() == StringVariant) {
        s = *boost::get<std::string>(&v_);
    }
    else {
        s = str();
    }
}

} // namespace Tempus
