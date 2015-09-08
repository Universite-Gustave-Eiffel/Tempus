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

#ifndef TEMPUS_ABSCISSA_HH
#define TEMPUS_ABSCISSA_HH

#include <istream>
#include <boost/assert.hpp>

namespace Tempus
{
///
/// double in the [0,1] range
class Abscissa
{
public:
    Abscissa();
    Abscissa( float v );
    operator float() const;
private:
    // stored on 16 bits
    uint16_t d_;
};

std::istream& operator>>( std::istream& istr, Abscissa& a );

}

#endif
