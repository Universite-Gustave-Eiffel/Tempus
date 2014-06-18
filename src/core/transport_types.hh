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

//
// This file contains common declarations and constants used by all the objects inside the "Tempus" namespace
//

#ifndef TEMPUS_TRANSPORT_TYPES_HH
#define TEMPUS_TRANSPORT_TYPES_HH

#include <map>
#include <string>
#include "common.hh"

namespace Tempus
{
///
/// Refers to tempus.transport_type table
struct TransportType : public Base {
    // inherits from Base, the ID must be a power of 2.
    db_id_t parent_id;

    std::string name;

    bool need_parking;
    bool need_station;
    bool need_return;
    bool need_network;

protected:
    bool check_consistency_() {
        ///
        /// x is a power of two if (x & (x - 1)) is 0
        REQUIRE( ( db_id != 0 ) && !( db_id & ( db_id - 1 ) ) );
        REQUIRE( ( parent_id != 0 ) && !( parent_id & ( parent_id - 1 ) ) );
        return true;
    }
};

///
/// Transport types constants.
typedef std::map<db_id_t, TransportType> TransportTypes;

}

#endif
