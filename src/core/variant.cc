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

namespace Tempus {

Variant::Variant( bool b ) : type_( BoolVariant )
{
    str_ = b ? "true" : "false";
}
Variant::Variant( int i ) : type_( IntVariant )
{
    std::ostringstream ostr;
    ostr << i;
    str_ = ostr.str();
}
Variant::Variant( double i ) : type_( FloatVariant )
{
    std::ostringstream ostr;
    ostr << i;
    str_ = ostr.str();
}
Variant::Variant( const std::string& s, VariantType t ) : type_( t ), str_( s ) {}

} // namespace Tempus
