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

#ifndef TEMPUS_IO_HH
#define TEMPUS_IO_HH

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/thread.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

///
/// @brief Macro for mutex protected cerr and cout
/// @detailled Macro creates a temporary responsible
/// for freeing the mutex at the end of the line (temporaries are destroyed at the ";")
/// the use of a macro allows to print __FILE__ and __LINE__ to
/// find easilly where à given print is done.
///

#define IOSTREAM_OUTPUT_LOCATION
#ifdef IOSTREAM_OUTPUT_LOCATION
#   define TEMPUS_LOCATION __FILE__ << ":" << __LINE__ << " "
#else
#   define TEMPUS_LOCATION ""
#endif

#define CERR if(1) (boost::lock_guard<boost::mutex>( Tempus::iostream_mutex ), std::cerr << TEMPUS_LOCATION )
#define COUT if(1) (boost::lock_guard<boost::mutex>( Tempus::iostream_mutex ), std::cout << TEMPUS_LOCATION )

namespace Tempus
{
extern boost::mutex iostream_mutex; // its a plain old global variable
}

#endif
