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

#ifndef TEMPUS_OPTIONAL_HH
#define TEMPUS_OPTIONAL_HH

#include <limits>

namespace Tempus
{

///
/// Numeric type with one sacrified value used as "null"
/// Inspired by boost::optional, but with smallest memory footprint
template<typename T, T NullValue = std::numeric_limits<T>::max()>
class Optional
{
public:
    Optional() : v_(NullValue) {}
    Optional( T t ) : v_(t) {}
    T operator *() const { return v_; }
    operator bool() const { return v_ == NullValue; }
private:
    T v_;
};

}

#endif
