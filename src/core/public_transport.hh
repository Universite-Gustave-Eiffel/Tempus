/**
 *   Copyright (C) 2012-2015 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
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

#ifndef TEMPUS_PUBLIC_TRANSPORT_HH
#define TEMPUS_PUBLIC_TRANSPORT_HH

#include "property.hh"

namespace Tempus {
namespace PublicTransport {

///
/// Public transport agency
class Agency : public Base
{
public:
    DECLARE_RW_PROPERTY( name, std::string );
};
typedef std::vector<Agency> AgencyList;

///
/// Public transport networks. A network can be made of several agencies
struct Network : public Base {
    DECLARE_RW_PROPERTY( name, std::string );

public:
    ///
    /// Get the list of agencies of this network
    const AgencyList& agencies() const { return agencies_; }

    ///
    /// Add an agency to this network
    void add_agency( const Agency& agency );
private:
    AgencyList agencies_;
};

} // PublicTransport namespace

} // Tempus namespace

#endif
