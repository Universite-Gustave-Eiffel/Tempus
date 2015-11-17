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

#include <iostream>
#include <fstream>

#include "application.hh"
#include "common.hh"
#include "config.hh"

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#endif

#if ENABLE_SEGMENT_ALLOCATOR
#include "utils/segment_allocator.hh"
#endif

Tempus::Application* get_application_instance_()
{
    return Tempus::Application::instance();
}

namespace Tempus {

Application::Application()
{
}

Application::~Application()
{
#if ENABLE_SEGMENT_ALLOCATOR
    SegmentAllocator::release();
#endif
}

Application* Application::instance()
{
    // On Windows, static and global variables are COPIED from the main module (EXE) to the other (DLL).
    // DLL have still access to the main EXE memory ...
    static Application* instance_ = 0;

    if ( 0 == instance_ ) {
#ifdef _WIN32
        // We test if we are in the main module (EXE) or not. If it is the case, a new Application is allocated.
        // It will also be returned by modules.
        Application* ( *main_get_instance )() = ( Application* (* )() )GetProcAddress( GetModuleHandle( NULL ), "get_application_instance_" );

        if ( main_get_instance == &get_application_instance_ ) {
            instance_ = new Application();
        }
        else {
            instance_ = main_get_instance();
        }

#else
        instance_ = new Application();
#endif
    }

    return instance_;
}

void Application::set_option( const std::string& key, const Variant& value )
{
    options_[key] = value;
}

Variant Application::option( const std::string& key ) const
{
    auto it = options_.find( key );
    if ( it == options_.end() ) {
        return Variant(std::string(""));
    }
    return it->second;
}

void Application::set_data_directory( const std::string& d )
{
    set_option( "data_directory", Variant::from_string(d) );
}

const std::string Application::data_directory() const
{
    std::string data_dir;
    auto it = options_.find( "data_directory" );
    if ( it != options_.end() ) {
        data_dir = it->second.str();
    }
    else {
        const char* d = getenv( "TEMPUS_DATA_DIRECTORY" );

        if ( !d ) {
            const std::string msg = "environment variable TEMPUS_DATA_DIRECTORY is not defined";
            CERR << msg << "\n";
            throw std::runtime_error( msg );
        }
        data_dir = d;
    }

    // remove trailing space in path (windows will do that for you, an the env var
    // defined in visual studio has that space which screws up concatenation)
    data_dir.erase( data_dir.find_last_not_of( " " )+1 );
    return data_dir;
}


}
