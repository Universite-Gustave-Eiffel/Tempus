// Tempus WPS server
// (c) 2012 Oslandia
// MIT License
//
// standard stream headers must be included first, to please VC++
#include <iostream>
#include <fstream>

#include <string>
#include <boost/thread.hpp>

#include <fcgi_stdio.h>
#include <fcgio.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "wps_service.hh"
#include "wps_request.hh"
#include "application.hh"
#include "xml_helper.hh"

#ifdef _WIN32
#define chdir _chdir
#define environ _environ
#endif

#define DEBUG_TRACE if(1) std::cout << " debug: "
using namespace std;

///
/// This is the main WPS server. It is designed to be called by a FastCGI-aware web server
///

struct RequestThread
{
    RequestThread( int listen_socket )
        :_listen_socket(listen_socket)
    {
    }

    void operator ()(void) const // noexept
    {
        FCGX_Request request;
        FCGX_InitRequest(&request, _listen_socket, 0);
        XML::init(); // must be called once per thread

        for (;;)
        {
            if ( FCGX_Accept_r(&request) )
            {
                CERR << "failed to accept request\n";
                FCGX_Finish_r(&request);
                continue;
            }

            try 
            {
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
            catch (std::exception & e)
            {
                CERR << "failed to process request (excpetion thrown): " << e.what() << "\n";
            }

            FCGX_Finish_r(&request);
        }
    }

private:
    const int _listen_socket;
};


int main( int argc, char*argv[] )
{
    xmlInitParser();
    bool standalone = false;
    // the default TCP port to listen to
    string port_str = ""; // ex 9000
    string chdir_str = "";
    std::vector<string> plugins;
    size_t num_threads = 1;
    string dbstring = "dbname=tempus_test_db";
    if (argc > 1 )
    {
	for ( int i = 1; i < argc; i++ )
	{
	    string arg = argv[i];
	    if ( arg == "-p" )
	    {
		standalone = true;
		if ( argc > i+1 )
		{
		    port_str = argv[++i];
		}
	    }
	    else if ( arg == "-c" )
	    {
		if ( argc > i+1 )
		{
		    chdir_str = argv[++i];
		}
	    }
	    else if ( arg == "-l" )
	    {
		if ( argc > i+1 )
		{
		    plugins.push_back( argv[++i] );
		}
	    }
	    else if ( arg == "-t" )
	    {
		if ( argc > i+1 )
		{
		    num_threads = atoi(argv[++i]);
		}
	    }
	    else if ( arg == "-d" )
	    {
		if ( argc > i+1 )
		{
		    dbstring = argv[++i];
		}
	    }
	    else
	    {
		std::cout << "Options: " << endl
		    << "\t-p port_number\tstandalone mode (for use with nginx and lighttpd)" << endl
		    << "\t-c dir\tchange directory to dir before execution" << endl
		    << "\t-t num_threads\tnumber of request-processing threads" << endl
		    << "\t-l plugin_name\tload plugin" << endl
		    << "\t-d dbstring\tstring used to connect to pgsql" << endl
		    ;
		return (arg == "-h") ? EXIT_SUCCESS : EXIT_FAILURE;
	    }
	}
    }
    
    if ( chdir_str != "" )
    {
        if( chdir( chdir_str.c_str() ) )
        {
            std::cerr << "cannot change directory to " << chdir_str << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    FCGX_Init();

    // initialise connection and graph
    try
    {
        std::cout << "connecting to database: " << dbstring << "\n";
        Tempus::Application::instance()->connect( dbstring );
        Tempus::Application::instance()->pre_build_graph();
        std::cout << "buiding the graph...\n";
        Tempus::Application::instance()->build_graph();
        // load plugins
        for ( size_t i=0; i< plugins.size(); i++ )
        {
            using namespace Tempus;
			std::cerr << "loading " << plugins[i] << "\n";
            PluginFactory::instance()->load( plugins[i] );
        }
    }
    catch ( std::exception & e )
    {
            std::cerr << "cannot create application: " << e.what() << std::endl;
            return EXIT_FAILURE;
    }

    std::cout << "ready !\n";

    int listen_socket = 0;
    if ( standalone )
    {
        listen_socket = FCGX_OpenSocket((":" + port_str).c_str(), 10);
        if ( listen_socket < 0 )
        {
            std::cerr << "Problem opening the socket on port " << port_str << std::endl;
            return EXIT_FAILURE;
        }
    }

    try{
        boost::thread_group pool; 
        for (size_t i=0; i<num_threads-1; i++) 
            pool.add_thread( new boost::thread( RequestThread( listen_socket ) ) );

        RequestThread mainThread(listen_socket); // so we use the main
        mainThread();
        pool.join_all();
    }
    catch ( std::exception& e )
    {
       std::cerr << "exception during thread creation or deletion: " << e.what() << std::endl;
       return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
