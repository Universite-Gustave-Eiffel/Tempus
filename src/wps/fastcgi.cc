// standard stream headers must be included first, to please VC++
#include <iostream>
#include <fstream>

#include <string>

#include <fcgi_stdio.h>
#include <fcgio.h>
#include <direct.h>

#include "wps_service.hh"
#include "wps_request.hh"
#include "application.hh"

#ifdef _WIN32
#define chdir _chdir
#endif

using namespace std;

int main( int argc, char*argv[] )
{
	bool standalone = false;
	string port_str = "9000";
	string chdir_str = "";
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
			else
			{
				cerr << "Options: " << endl;
				cerr << "\t-p port_number\tstandalone mode (for use with nginx and lighttpd)" << endl;
				cerr << "\t-c dir\tchange directory to dir before execution" << endl;
				return 1;
			}
		}
	}

	if ( chdir_str != "" )
	{
		chdir( chdir_str.c_str() );
	}
	
    Tempus::Application* app = Tempus::Application::instance();
    app->load_plugin( "sample_road_plugin" );
    app->load_plugin( "sample_pt_plugin" );

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
	fcgi_streambuf cout_fcgi_streambuf( request.out );

	environ = request.envp;

	WPS::Request wps_request( &cin_fcgi_streambuf, &cout_fcgi_streambuf );
	wps_request.process();
    }

    return 0;
}
