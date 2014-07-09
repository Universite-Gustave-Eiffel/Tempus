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

#include "application.hh"
#include "common.hh"
#include "pgsql_importer.hh"

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#endif

Tempus::Application* get_application_instance_()
{
    return Tempus::Application::instance();
}

namespace Tempus {
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
            instance_->state_ = Started;
        }
        else {
            instance_ = main_get_instance();
        }

#else
        instance_ = new Application();
        instance_->state_ = Started;
#endif
    }

    return instance_;
}

void Application::connect( const std::string& ddb_options )
{
    db_options_ = ddb_options;
    state_ = Connected;
}

void Application::pre_build_graph()
{
    state_ = GraphPreBuilt;
}

void Application::build_graph( bool consistency_check )
{
    // request the database
    PQImporter importer( db_options_ );
    TextProgression progression( 50 );
    COUT << "Loading graph from database: " << std::endl;
    COUT << "Importing constants ... " << std::endl;
    importer.import_constants( graph_ );
    COUT << "Importing graph ... " << std::endl;
    importer.import_graph( graph_, progression, consistency_check );
    state_ = GraphBuilt;
}

const std::string Application::data_directory() const
{
    const char* data_dir = getenv( "TEMPUS_DATA_DIRECTORY" );

    if ( !data_dir ) {
        const std::string msg = "environment variable TEMPUS_DATA_DIRECTORY is not defined";
        CERR << msg << "\n";
        throw std::runtime_error( msg );
    }

    // remove trailing space in path (windows will do that for you, an the env var
    // defined in visual studio has that space wich screws up concatenation)
    std::string dir( data_dir );
    dir.erase( dir.find_last_not_of( " " )+1 );
    return dir;
}
}
