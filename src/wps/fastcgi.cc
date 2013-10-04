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

#ifdef _WIN32
#define chdir _chdir
#define environ _environ
#endif

#define DEBUG_TRACE if(1) (std::cout << __FILE__ << ":" << __LINE__ << " debug: ")
using namespace std;

///
/// This is the main WPS server. It is designed to be called by a FastCGI-aware web server
///

boost::mutex mutex;

struct RequestThread
{
    RequestThread( int listen_socket )
        :_listen_socket(listen_socket)
    {
    }

    void operator ()(void)
    {
        FCGX_Request request;
        FCGX_InitRequest(&request, _listen_socket, 0);

        DEBUG_TRACE << "listening\n";

        for (;;)
        {
            {
                boost::lock_guard< boost::mutex > lock(mutex);
                DEBUG_TRACE << "accepting\n";
                if ( FCGX_Accept_r(&request) ) throw std::runtime_error("error in accept");
            }
            fcgi_streambuf cin_fcgi_streambuf( request.in );
            // This causes a crash under Windows (??). We rely on a classic stringstream and FCGX_PutStr
            // fcgi_streambuf cout_fcgi_streambuf( request.out );
            std::ostringstream outbuf;

            //environ = request.envp;

            WPS::Request wps_request( &cin_fcgi_streambuf, outbuf.rdbuf(), request.envp );
            assert( wps_request.getParam("REQUEST_METHOD") );

            wps_request.process();
            const std::string& outstr = outbuf.str();
            FCGX_PutStr( outstr.c_str(), outstr.size(), request.out );
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
	    else
	    {
		cerr << "Options: " << endl;
		cerr << "\t-p port_number\tstandalone mode (for use with nginx and lighttpd)" << endl;
		cerr << "\t-c dir\tchange directory to dir before execution" << endl;
		cerr << "\t-t num_threads\tnumber of request-processing threads" << endl;
		cerr << "\t-l plugin_name\tload plugin" << endl;
		return 1;
	    }
	}
    }
    
    if ( chdir_str != "" )
    {
        int c = chdir( chdir_str.c_str() );
    }
    
    FCGX_Request request;

    FCGX_Init();


    // initialise connection and graph
    Tempus::Application::instance()->connect( "dbname=tempus_test_db" );
    Tempus::Application::instance()->state( Tempus::Application::Connected );
    Tempus::Application::instance()->pre_build_graph();
    Tempus::Application::instance()->state( Tempus::Application::GraphPreBuilt );
    Tempus::Application::instance()->build_graph();
    Tempus::Application::instance()->state( Tempus::Application::GraphBuilt );

    int listen_socket = 0;
    if ( standalone )
    {
        listen_socket = FCGX_OpenSocket((":" + port_str).c_str(), 10);
        if ( listen_socket < 0 )
        {
            cerr << "Problem opening the socket on port " << port_str << std::endl;
            return EXIT_FAILURE;
        }
    }

    try{
        // load plugins
        for ( size_t i=0; i< plugins.size(); i++ )
        {
            using namespace Tempus;
            PluginFactory::instance.load( plugins[i] );
        }

        boost::thread_group pool; 
        for (size_t i=0; i<num_threads; i++) 
            pool.add_thread( new boost::thread( RequestThread( listen_socket ) ) );
        pool.join_all();

    }
    catch ( std::exception& e )
    {
        cerr << "Exception during WPS execution: " << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
