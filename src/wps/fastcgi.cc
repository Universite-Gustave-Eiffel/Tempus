#include <fcgi_stdio.h>
#include <fcgio.h>

#include "wps_service.hh"
#include "plugin.hh"

using namespace std;

// defined in wps_server.cc
int process_request();

int main()
{
    Tempus::Plugin* plugin = Tempus::Plugin::load( "sample_road_plugin" );
    //    Tempus::Plugin* plugin = Tempus::Plugin::load( "dummy_plugin" );
    WPS::Service::set_plugin( plugin );

    FCGX_Request request;

    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);

    while ( FCGX_Accept_r(&request) == 0 )
    {
	fcgi_streambuf cin_fcgi_streambuf( request.in );
	fcgi_streambuf cout_fcgi_streambuf( request.out );

	cin.rdbuf( &cin_fcgi_streambuf );
	cout.rdbuf( &cout_fcgi_streambuf );

	environ = request.envp;

	process_request();
    }

    return 0;
}
