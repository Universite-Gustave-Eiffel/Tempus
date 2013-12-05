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

#include <string>
#include <iostream>
#include <memory>

#include <boost/format.hpp>

#include "plugin.hh"
#include "utils/graph_db_link.hh"
#ifdef _WIN32
#include <strsafe.h>
#endif

Tempus::PluginFactory* get_plugin_factory_instance_()
{
    return Tempus::PluginFactory::instance();
}

namespace Tempus {


PluginFactory* PluginFactory::instance()
{
    // On Windows, static and global variables are COPIED from the main module (EXE) to the other (DLL).
    // DLL have still access to the main EXE memory ...
    static PluginFactory* instance_ = 0;

    if ( 0 == instance_ ) {
#ifdef _WIN32
        // We test if we are in the main module (EXE) or not. If it is the case, a new Application is allocated.
        // It will also be returned by modules.
        PluginFactory * ( *main_get_instance )() = ( PluginFactory* (* )() )GetProcAddress( GetModuleHandle( NULL ), "get_plugin_factory_instance_" );
        instance_ = ( main_get_instance == &get_plugin_factory_instance_ )  ? new PluginFactory : main_get_instance();
#else
        instance_ = new PluginFactory();
#endif
    }

    return instance_;
}


#ifdef _WIN32
std::string win_error()
{
    // Retrieve the system error message for the last-error code
    struct RAII {
        LPVOID ptr;
        RAII( LPVOID p ):ptr( p ) {};
        ~RAII() {
            LocalFree( ptr );
        }
    };
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        ( LPTSTR ) &lpMsgBuf,
        0, NULL );
    RAII lpMsgBufRAII( lpMsgBuf );

    // Display the error message and exit the process

    LPVOID lpDisplayBuf = ( LPVOID )LocalAlloc( LMEM_ZEROINIT,
                          ( lstrlen( ( LPCTSTR )lpMsgBuf ) + 40 ) * sizeof( TCHAR ) );
    RAII lpDisplayBufRAII( lpDisplayBuf );
    StringCchPrintf( ( LPTSTR )lpDisplayBuf,
                     LocalSize( lpDisplayBuf ) / sizeof( TCHAR ), TEXT( "failed with error %d: %s" ), dw, lpMsgBuf );
    return std::string( ( const char* )lpDisplayBuf );
}
#endif

PluginFactory::~PluginFactory()
{
    for ( DllMap::iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
#ifdef _WIN32
        FreeLibrary( i->second.handle_ );
#else

        if ( dlclose( i->second.handle_ ) ) {
            CERR << "Error on dlclose " << dlerror() << std::endl;
        }

#endif
    }
}

void PluginFactory::load( const std::string& dll_name )
{
    if ( dll_.find( dll_name ) != dll_.end() ) {
        return;    // already there
    }

    const std::string complete_dll_name = DLL_PREFIX + dll_name + DLL_SUFFIX;

    COUT << "Loading " << complete_dll_name << std::endl;

#ifdef _WIN32
#   define DLCLOSE FreeLibrary
#   define DLSYM GetProcAddress
#   define THROW_DLERROR( msg )  throw std::runtime_error( std::string( msg ) + "(" + win_error() + ")")
#else
#   define DLCLOSE dlclose
#   define DLSYM dlsym
#   define THROW_DLERROR( msg ) throw std::runtime_error( std::string( msg ) + "(" + std::string( dlerror() ) + ")" );
#endif
    struct RAII {
        RAII( HMODULE h ):h_( h ) {};
        ~RAII() {
            if ( h_ ) {
                DLCLOSE( h_ );
            }
        }
        HMODULE get() {
            return h_;
        }
        HMODULE release() {
            HMODULE p = NULL;
            std::swap( p,h_ );
            return p;
        }
    private:
        HMODULE h_;
    };

    RAII hRAII(
#ifdef _WIN32
        LoadLibrary( complete_dll_name.c_str() )
#else
        dlopen( complete_dll_name.c_str(), RTLD_NOW | RTLD_GLOBAL )
#endif
    );

    if ( !hRAII.get() ) {
        THROW_DLERROR( "cannot load " + complete_dll_name );
    }

    Dll::PluginCreationFct createFct;
    // this cryptic syntax is here to avoid a warning when converting
    // a pointer-to-function to a pointer-to-object
    *reinterpret_cast<void**>( &createFct ) = DLSYM( hRAII.get(), "createPlugin" );

    if ( !createFct ) {
        THROW_DLERROR( "no function createPlugin in " + complete_dll_name );
    }

    Dll::PluginOptionDescriptionFct optDescFct;
    *reinterpret_cast<void**>( &optDescFct ) = DLSYM( hRAII.get(), "optionDescriptions" );

    if ( !optDescFct ) {
        THROW_DLERROR( "no function optionDescriptions in " + complete_dll_name  );
    }

    Dll::PluginNameFct nameFct;
    *reinterpret_cast<void**>( &nameFct ) = DLSYM( hRAII.get(), "pluginName" );

    if ( nameFct == NULL ) {
        THROW_DLERROR( "no function pluginName in " + complete_dll_name  );
    }

    typedef void ( *PluginPostBuildFct )();
    PluginPostBuildFct postBuildFct;
    *reinterpret_cast<void**>( &postBuildFct ) = DLSYM( hRAII.get(), "post_build" );

    if ( nameFct == NULL ) {
        THROW_DLERROR( "no function post_build in " + complete_dll_name  );
    }

    Dll dll = { hRAII.release(), createFct, optDescFct };
    const std::string pluginName = ( *nameFct )();
    dll_.insert( std::make_pair( pluginName, dll ) );

    if ( Application::instance()->state() < Application::GraphBuilt ) {
        throw std::runtime_error( "trying to load plugin (post_build graph) while the graph has not been build" );
    }

    postBuildFct();
    COUT << "loaded " << pluginName << " from " << dll_name << "\n";
}

std::vector<std::string> PluginFactory::plugin_list() const
{
    std::vector<std::string> names;

    for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
        names.push_back( i->first );
    }

    return names;
}

Plugin* PluginFactory::createPlugin( const std::string& dll_name ) const
{
    std::string loaded;

    for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
        loaded += " " + i->first;
    }

    DllMap::const_iterator dll = dll_.find( dll_name );

    if ( dll == dll_.end() ) {
        throw std::runtime_error( dll_name + " is not loaded (loaded:" + loaded+")" );
    }

    std::auto_ptr<Plugin> p( dll->second.create( Application::instance()->db_options() ) );
    p->validate();
    return p.release();
}

const Plugin::OptionDescriptionList PluginFactory::option_descriptions( const std::string& dll_name ) const
{
    std::string loaded;

    for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
        loaded += " " + i->first;
    }

    DllMap::const_iterator dll = dll_.find( dll_name );

    if ( dll == dll_.end() ) {
        throw std::runtime_error( dll_name + " is not loaded (loaded:" + loaded+")" );
    }

    std::auto_ptr<const Plugin::OptionDescriptionList> list( dll->second.options_description() );
    return Plugin::OptionDescriptionList( *list );
}


Plugin::Plugin( const std::string& nname, const std::string& db_options ) :
    graph_( Application::instance()->graph() ),
    name_( nname ),
    db_( db_options ) // create another connection
{
    // default metrics
    metrics_[ "time_s" ] = ( double )0.0;
    metrics_[ "iterations" ] = ( int )0;
}

Plugin::OptionValue::OptionValue( bool b ) : type_( BoolOption )
{
    str_ = b ? "true" : "false";
}
Plugin::OptionValue::OptionValue( int i ) : type_( IntOption )
{
    std::ostringstream ostr;
    ostr << i;
    str_ = ostr.str();
}
Plugin::OptionValue::OptionValue( double i ) : type_( FloatOption )
{
    std::ostringstream ostr;
    ostr << i;
    str_ = ostr.str();
}
Plugin::OptionValue::OptionValue( const std::string& s, OptionType t ) : type_( t ), str_( s ) {}

template <class T>
void Plugin::get_option( const std::string& nname, T& value )
{
    const OptionDescriptionList desc = PluginFactory::instance()->option_descriptions( name_ );
    OptionDescriptionList::const_iterator descIt = desc.find( nname );

    if ( descIt == desc.end() ) {
        throw std::runtime_error( "get_option(): cannot find option " + nname );
    }

    if ( options_.find( nname ) == options_.end() ) {
        // return default value
        value = descIt->second.default_value.as<T>();
        return;
    }

    value = options_[nname].as<T>();
}
// template instanciations
template void Plugin::get_option<bool>( const std::string&, bool& );
template void Plugin::get_option<int>( const std::string&, int& );
template void Plugin::get_option<double>( const std::string&, double& );
template void Plugin::get_option<std::string>( const std::string&, std::string& );

void Plugin::set_option_from_string( const std::string& nname, const std::string& value, Plugin::OptionType type )
{
    const Plugin::OptionDescriptionList desc = PluginFactory::instance()->option_descriptions( name_ );
    Plugin::OptionDescriptionList::const_iterator descIt = desc.find( nname );

    if ( descIt == desc.end() ) {
        return;
    }

    const OptionType t = descIt->second.type();

    if ( t != type ) {
        throw std::invalid_argument( ( boost::format( "Requested type %1% for option %2% is different from the declared type %3%" )
                                       % type
                                       % nname
                                       % t ).str() );
    }

    options_[nname] = OptionValue( value, t );
}

std::string Plugin::option_to_string( const std::string& nname )
{
    const Plugin::OptionDescriptionList desc = PluginFactory::instance()->option_descriptions( name_ );
    Plugin::OptionDescriptionList::const_iterator descIt = desc.find( nname );

    if ( descIt == desc.end() ) {
        throw std::invalid_argument( "Cannot find option " + nname );
    }

    OptionValue value = options_[nname];
    return value.str();
}

std::string Plugin::metric_to_string( const std::string& nname )
{
    if ( metrics_.find( nname ) == metrics_.end() ) {
        throw std::invalid_argument( "Cannot find metric " + nname );
    }

    MetricValue v = metrics_[nname];
    return v.str();
}

void Plugin::validate()
{
    COUT << "[plugin_base]: validate" << std::endl;
}

void Plugin::cycle()
{
    COUT << "[plugin_base]: cycle" << std::endl;
}

void Plugin::pre_process( Request& request )
{
    COUT << "[plugin_base]: pre_process" << std::endl;
    request_ = request;
    result_.clear();
}

///
/// Process the user request.
/// Must populates the 'result_' object.
void Plugin::process()
{
    COUT << "[plugin_base]: process" << std::endl;
}

void Plugin::post_process()
{
    COUT << "[plugin_base]: post_process" << std::endl;
}

///
/// Text formatting and preparation of roadmap
Result& Plugin::result()
{
    for ( Result::iterator rit = result_.begin(); rit != result_.end(); ++rit ) {
        Roadmap& roadmap = *rit;
        Road::Graph& road_graph = graph_.road;

        Tempus::db_id_t previous_section = 0;
        bool on_roundabout = false;
        bool was_on_roundabout = false;
        Roadmap::RoadStep* last_step = 0;

        Roadmap::RoadStep::EndMovement movement;

        for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++ ) {
            if ( it->step_type == Roadmap::Step::GenericStep ) {
                Roadmap::GenericStep* step = static_cast<Roadmap::GenericStep*>( &*it );
                Multimodal::Edge* edge = static_cast<Multimodal::Edge*>( step );

                bool is_road_pt = false;
                db_id_t road_id, pt_id;

                switch ( edge->connection_type() ) {
                case Multimodal::Edge::Road2Transport: {
                    is_road_pt = true;
                    const Road::Graph& rroad_graph = *( edge->source.road_graph );
                    const PublicTransport::Graph& pt_graph = *( edge->target.pt_graph );
                    road_id = rroad_graph[ edge->source.road_vertex ].db_id;
                    pt_id = pt_graph[ edge->target.pt_vertex ].db_id;
                }
                break;
                case Multimodal::Edge::Transport2Road: {
                    is_road_pt = true;
                    const PublicTransport::Graph& pt_graph = *( edge->source.pt_graph );
                    const Road::Graph& rroad_graph = *( edge->target.road_graph );
                    pt_id = pt_graph[ edge->source.pt_vertex ].db_id;
                    road_id = rroad_graph[ edge->target.road_vertex ].db_id;
                }
                break;
                default:
                    break;
                }

                if ( is_road_pt ) {
                    std::string query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
                                                         "(select geom from tempus.road_node where id=%1%) as t1, "
                                                         "(select geom from tempus.pt_stop where id=%2%) as t2" ) %
                                          road_id %
                                          pt_id ).str();
                    Db::Result res = db_.exec( query );
                    BOOST_ASSERT( res.size() > 0 );
                    std::string wkb = res[0][0].as<std::string>();
                    // get rid of the heading '\x'
                    step->geometry_wkb = wkb.substr( 2 );
                }
            }
            else if ( it->step_type == Roadmap::Step::PublicTransportStep ) {
                Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( &*it );
                PublicTransport::Graph& pt_graph = graph_.public_transports[step->network_id];

                //
                // retrieval of the step's geometry
                std::string q = ( boost::format( "SELECT st_asbinary(geom) FROM tempus.pt_section WHERE stop_from=%1% AND stop_to=%2%" ) %
                                  pt_graph[step->section].stop_from % pt_graph[step->section].stop_to ).str();
                Db::Result res = db_.exec( q );
                std::string wkb = res[0][0].as<std::string>();
                // get rid of the heading '\x'
                step->geometry_wkb = wkb.substr( 2 );
            }
            else if ( it->step_type == Roadmap::Step::RoadStep ) {

                Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( &*it );

                //
                // retrieval of the step's geometry
                {
                    // reverse the geometry if needed
                    std::string q = ( boost::format( "SELECT CASE WHEN node_from=%1%"
                                                     " THEN ST_AsBinary(geom)"
                                                     " ELSE ST_AsBinary(ST_Reverse(geom)) END"
                                                     " FROM tempus.road_section WHERE id=%2%" ) %
                                      road_graph[ source(road_graph[step->road_section].edge, road_graph) ].db_id %
                                      road_graph[step->road_section].db_id ).str();
                    Db::Result res = db_.exec( q );
                    std::string wkb = res[0][0].as<std::string>();

                    // get rid of the heading '\x'
                    if ( wkb.size() > 0 ) {
                        step->geometry_wkb = wkb.substr( 2 );
                    }
                    else {
                        step->geometry_wkb = "";
                    }
                }

                //
                // For a road step, we have to compute directions of turn
                //
                movement = Roadmap::RoadStep::GoAhead;

                on_roundabout =  road_graph[step->road_section].is_roundabout;

                bool action = false;

                if ( on_roundabout && !was_on_roundabout ) {
                    // we enter a roundabout
                    movement = Roadmap::RoadStep::RoundAboutEnter;
                    action = true;
                }

                if ( !on_roundabout && was_on_roundabout ) {
                    // we leave a roundabout
                    // FIXME : compute the exit number
                    movement = Roadmap::RoadStep::FirstExit;
                    action = true;
                }

                if ( previous_section && !on_roundabout && !action ) {
                    std::string q1 = ( boost::format( "SELECT ST_Azimuth( st_endpoint(s1.geom), st_startpoint(s1.geom) ), ST_Azimuth( st_startpoint(s2.geom), st_endpoint(s2.geom) ), st_endpoint(s1.geom)=st_startpoint(s2.geom) "
                                                      "FROM tempus.road_section AS s1, tempus.road_section AS s2 WHERE s1.id=%1% AND s2.id=%2%" ) % previous_section % road_graph[step->road_section].db_id ).str();
                    Db::Result res = db_.exec( q1 );
                    double pi = 3.14159265;
                    double z1 = res[0][0].as<double>() / pi * 180.0;
                    double z2 = res[0][1].as<double>() / pi * 180.0;
                    bool continuous =  res[0][2].as<bool>();

                    if ( !continuous ) {
                        z1 = z1 - 180;
                    }

                    int z = int( z1 - z2 );
                    z = ( z + 360 ) % 360;

                    if ( z >= 30 && z <= 150 ) {
                        movement = Roadmap::RoadStep::TurnRight;
                    }

                    if ( z >= 210 && z < 330 ) {
                        movement = Roadmap::RoadStep::TurnLeft;
                    }
                }

                if ( last_step ) {
                    last_step->end_movement = movement;
                    last_step->distance_km = -1.0;
                }

                previous_section = road_graph[step->road_section].db_id;
                was_on_roundabout = on_roundabout;
                last_step = step;
            }
        }

        if ( last_step ) {
            last_step->end_movement = Roadmap::RoadStep::YouAreArrived;
            last_step->distance_km = -1.0;
        }
    }

    return result_;
}

void Plugin::cleanup()
{
    COUT << "[plugin_base]: cleanup" << std::endl;
}
}

