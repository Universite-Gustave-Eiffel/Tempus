#include <fcgi_stdio.h>
#include <fcgio.h>

#include "wps_service.hh"
#include "wps_request.hh"
#include "plugin.hh"

using namespace std;

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

	environ = request.envp;

	WPS::Request wps_request( &cin_fcgi_streambuf, &cout_fcgi_streambuf );
	wps_request.process();
    }

    return 0;
}
