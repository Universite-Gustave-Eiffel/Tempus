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

// Tempus WPS server
//
// standard stream headers must be included first, to please VC++
#include <iostream>
#include <fstream>

#include <string>
#include <boost/thread.hpp>


#ifdef _WIN32
#   include <direct.h>
#   define chdir _chdir
#   define environ _environ
#else
#   include <sys/types.h>// for umask
#   include <sys/stat.h> // for umask
#   define NO_FCGI_DEFINES
#endif

#include <fcgi_stdio.h>
#include <fcgio.h>

#include "wps_service.hh"
#include "wps_request.hh"
#include "application.hh"
#include "xml_helper.hh"


#define DEBUG_TRACE if(1) std::cout << " debug: "
using namespace std;

///
/// This is the main WPS server. It is designed to be called by a FastCGI-aware web server
///

struct RequestThread {
    RequestThread( int listen_socket )
        :_listen_socket( listen_socket ) {
    }

    void operator ()( void ) const { // noexept
        FCGX_Request request;
        FCGX_InitRequest( &request, _listen_socket, 0 );
        XML::init(); // must be called once per thread

        for ( ;; ) {
            if ( FCGX_Accept_r( &request ) ) {
                CERR << "failed to accept request\n";
                FCGX_Finish_r( &request );
                continue;
            }

            try {
                fcgi_streambuf cin_fcgi_streambuf( request.in );

                // This causes a crash under Windows (??).
                // We rely on a classic stringstream and FCGX_PutStr
                // fcgi_streambuf cout_fcgi_streambuf( request.out );
                std::ostringstream outbuf;

                WPS::Request wps_request( &cin_fcgi_streambuf, outbuf.rdbuf(), request.envp );

                wps_request.process();
                const std::string& outstr = outbuf.str();
                FCGX_PutStr( outstr.c_str(), outstr.size(), request.out );
            }
            catch ( std::exception& e ) {
                CERR << "failed to process request (excpetion thrown): " << e.what() << "\n";
            }

            FCGX_Finish_r( &request );
        }
    }

private:
    const int _listen_socket;
};


int main( int argc, char* argv[] )
{
    xmlInitParser();
    bool standalone = true;
    // the default TCP port to listen to
    string port_str = "9000"; // ex 9000
    string chdir_str = "";
    std::vector<string> plugins;
    size_t num_threads = 1;
    string dbstring = "dbname=tempus_test_db";
#ifndef WIN32
    bool daemon = false;
    ( void )daemon;
#endif

    if ( argc > 1 ) {
        for ( int i = 1; i < argc; i++ ) {
            string arg = argv[i];

            if ( arg == "-p" ) {
                //standalone = true;
                if ( argc > i+1 ) {
                    port_str = argv[++i];
                }
            }
            else if ( arg == "-c" ) {
                if ( argc > i+1 ) {
                    chdir_str = argv[++i];
                }
            }
            else if ( arg == "-l" ) {
                if ( argc > i+1 ) {
                    plugins.push_back( argv[++i] );
                }
            }

#ifndef WIN32
            else if ( arg == "-D" ) {
                daemon = true;
            }

#endif
            else if ( arg == "-t" ) {
                if ( argc > i+1 ) {
                    num_threads = atoi( argv[++i] );
                }
            }
            else if ( arg == "-d" ) {
                if ( argc > i+1 ) {
                    dbstring = argv[++i];
                }
            }
            else {
                std::cout << "Options: " << endl
                          << "\t-p port_number\tstandalone mode (for use with nginx and lighttpd)" << endl
                          << "\t-c dir\tchange directory to dir before execution" << endl
                          << "\t-t num_threads\tnumber of request-processing threads" << endl
                          << "\t-l plugin_name\tload plugin" << endl
                          << "\t-d dbstring\tstring used to connect to pgsql" << endl
#ifndef WIN32
                          << "\t-D\trun as daemon" << endl
#endif
                          ;
                return ( arg == "-h" ) ? EXIT_SUCCESS : EXIT_FAILURE;
            }
        }
    }

#ifndef WIN32

    if ( daemon ) {
        // Fork off the parent process
        const pid_t pid = fork();

        if ( pid < 0 ) {
            exit( EXIT_FAILURE );
        }

        // If we got a good PID, then we can exit the parent process.
        if ( pid > 0 ) {
            exit( EXIT_SUCCESS );
        }

        // Change the file mode mask
        umask( 0 );

        // redirect error an output strem to log files
        if (  !std::freopen( "/var/log/wps/err.log", "w", stderr )
                || !std::freopen( "/var/log/wps/out.log", "w", stdout ) ) {
            std::cerr << "error: cannot open /var/log/wps/err.log and /var/log/wps/out.log for writing\n";
            exit( EXIT_FAILURE );
        }

        const pid_t sid = setsid();

        if ( sid < 0 ) {
            std::cerr << "cannot setsid" << std::endl;
            exit( EXIT_FAILURE );
        }
    }

#endif

    if ( chdir_str != "" ) {
        if( chdir( chdir_str.c_str() ) ) {
            std::cerr << "cannot change directory to " << chdir_str << std::endl;
            return EXIT_FAILURE;
        }
    }

    FCGX_Init();

    // initialise connection and graph
    try {
        std::cout << "connecting to database: " << dbstring << "\n";
        Tempus::Application::instance()->connect( dbstring );
        Tempus::Application::instance()->pre_build_graph();
        std::cout << "building the graph...\n";
        Tempus::Application::instance()->build_graph();

        // load plugins
        for ( size_t i=0; i< plugins.size(); i++ ) {
            using namespace Tempus;
            std::cout << "loading " << plugins[i] << "\n";
            PluginFactory::instance()->load( plugins[i] );
        }
    }
    catch ( std::exception& e ) {
        std::cerr << "cannot create application: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "ready !" << std::endl;

    int listen_socket = 0;

    if ( standalone ) {
        listen_socket = FCGX_OpenSocket( ( ":" + port_str ).c_str(), 10 );

        if ( listen_socket < 0 ) {
            std::cerr << "Problem opening the socket on port " << port_str << std::endl;
            return EXIT_FAILURE;
        }
    }

    try {
        boost::thread_group pool;

        for ( size_t i=0; i<num_threads-1; i++ ) {
            pool.add_thread( new boost::thread( RequestThread( listen_socket ) ) );
        }

        RequestThread mainThread( listen_socket ); // so we use the main
        mainThread();
        pool.join_all();
    }
    catch ( std::exception& e ) {
        std::cerr << "exception during thread creation or deletion: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
