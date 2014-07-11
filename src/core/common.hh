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

#ifndef TEMPUS_COMMON_HH
#define TEMPUS_COMMON_HH

///
/// @mainpage TempusV2 API
///
/// TempusV2 is a framework which offers generic graph manipulation abilities in order to develop multimodal
/// path planning requests.
///
/// It is designed around a core, whose documentation is detailed here.
/// Main classes processed by TempusV2 are:
/// - Tempus::Road::Graph representing the road graph
/// - Tempus::PublicTransport::Graph representing a public transport graph
/// - Tempus::POI representing points of interest on the road graph
/// - Tempus::Multimodal::Graph which is a wrapper around a road graph, public transport graphs and POIs
///
/// These graphs are filled up with data coming from a database. Please refer to the Db namespace to see available functions.
/// Especially have a look at the Tempus::PQImporter class.
///
/// Path planning algorithms are designed to be written as user plugins. The Plugin base class gives access to some callbacks.
/// Please have a look at the three different sample plugins shipped with TempusV2: Tempus::RoadPlugin, Tempus::PtPlugin and Tempus::MultiPlugin.
///
/// The internal API is exposed to other programs and languages through a WPS server.
/// Have a look at the WPS::Service class and at its derived classes.
///

#include "cast.hh"
#include "io.hh"
#include "property.hh"
#include "cost.hh"
#include "base.hh"
#include "datetime.hh"
#include "progression.hh"

namespace Tempus {

#ifndef NDEBUG
///
/// Assertion, will abort if the condition is false
#define REQUIRE( expr ) {if (!(expr)) { std::stringstream ss; ss << __FILE__ << ":" << __LINE__ << ": Assertion " << #expr << " failed"; throw std::invalid_argument( ss.str() ); }}
#else
#define REQUIRE( expr ) ((void)0)
#endif

} // Tempus namespace

#endif
