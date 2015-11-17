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

#ifndef TEMPUS_APPLICATION_HH
#define TEMPUS_APPLICATION_HH

#include <string>

#include <boost/optional.hpp>

#include "multimodal_graph.hh"
#include "db.hh"
#include "variant.hh"

namespace Tempus {

///
/// Class used to represent the global state of the current application
class Application {
public:
    ///
    /// Access to the singleton instance
    static Application* instance();

    ~Application();

    ///
    /// Get the directory where data are stored
    const std::string data_directory() const;

    void set_option( const std::string& key, const Variant& value );
    Variant option( const std::string& ) const;

protected:
    // private constructor
    Application();

    std::map<std::string, Variant> options_;
};
}

#ifdef _WIN32
///
/// The global instance accessor must be callable as a C function under Windows
extern "C" __declspec( dllexport ) Tempus::Application* get_application_instance_();
#endif

#endif
