// standard stream headers must be included first, to please VC++
#include <iostream>
#include <fstream>

#include <string>

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

using namespace std;

int main( int argc, char*argv[] )
{
    bool standalone = false;
    string port_str = "9000";
    string chdir_str = "";
    std::vector<string> plugins;
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
	    else
	    {
		cerr << "Options: " << endl;
		cerr << "\t-p port_number\tstandalone mode (for use with nginx and lighttpd)" << endl;
		cerr << "\t-c dir\tchange directory to dir before execution" << endl;
		cerr << "\t-l plugin\tload plugin at startup" << endl;
		return 1;
	    }
	}
    }
    
    if ( chdir_str != "" )
    {
	int c = chdir( chdir_str.c_str() );
    }
    
    Tempus::Application* app = Tempus::Application::instance();
    for ( size_t i = 0; i < plugins.size(); i++ )
    {
	app->load_plugin( plugins[i] );
    }

    FCGX_Request request;

    FCGX_Init();
    if ( standalone )
    {
	int listen_socket;
	listen_socket = FCGX_OpenSocket((":" + port_str).c_str(), 10);
	if ( listen_socket < 0 )
	{
	    cerr << "Problem opening the socket" << endl;
	    return 2;
	}
	FCGX_InitRequest(&request, listen_socket, 0);
    }
    else
    {
	FCGX_InitRequest(&request, 0, 0);
    }
    
    while ( FCGX_Accept_r(&request) == 0 )
    {
	fcgi_streambuf cin_fcgi_streambuf( request.in );
	// This causes a crash under Windows (??). We rely on a classic stringstream and FCGX_PutStr
	// fcgi_streambuf cout_fcgi_streambuf( request.out );
	std::ostringstream outbuf;

	environ = request.envp;

	WPS::Request wps_request( &cin_fcgi_streambuf, outbuf.rdbuf() );
	wps_request.process();
	const std::string& outstr = outbuf.str();
	FCGX_PutStr( outstr.c_str(), outstr.size(), request.out );
    }

    return 0;
}
