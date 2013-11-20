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

///
/// This is a demo of a CLI program that uses the Tempus core and plugins to operate a path searching
///

#include <boost/program_options.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <iostream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;
using namespace Tempus;

namespace po = boost::program_options;

int main( int argc, char* argv[] )
{
    // default db options
    string db_options = "dbname=tempus_test_db";
    string plugin_name = "sample_road_plugin";
    Tempus::db_id_t origin_id = 21406;
    Tempus::db_id_t destination_id = 21015;

    // parse command line arguments
    po::options_description desc( "Allowed options" );
    desc.add_options()
    ( "help", "produce help message" )
    ( "db", po::value<string>(), "set database connection options" )
    ( "plugin", po::value<string>(), "set the plugin name to launch" )
    ( "origin", po::value<Tempus::db_id_t>(), "set the origin vertex id" )
    ( "destination", po::value<Tempus::db_id_t>(), "set the destination vertex id" )
    ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );

    if ( vm.count( "help" ) ) {
        COUT << desc << "\n";
        return 1;
    }

    if ( vm.count( "db" ) ) {
        db_options = vm["db"].as<string>();
    }

    if ( vm.count( "plugin" ) ) {
        plugin_name = vm["plugin"].as<string>();
    }

    if ( vm.count( "origin" ) ) {
        origin_id = vm["origin"].as<Tempus::db_id_t>();
    }

    if ( vm.count( "destination" ) ) {
        destination_id = vm["destination"].as<Tempus::db_id_t>();
    }

    Tempus::Application* app = Tempus::Application::instance();
    app->connect( db_options );

    ///
    /// Plugins
    try {
        std::auto_ptr<Plugin> plugin( PluginFactory::instance()->createPlugin( plugin_name ) );

        COUT << "[plugin " << plugin->name() << "]" << endl;

        COUT << endl << ">> pre_build" << endl;
        app->pre_build_graph();
        COUT << endl << ">> build" << endl;
        app->build_graph();

        //
        // Build the user request
        Multimodal::Graph& graph = app->graph();
        Road::Graph& road_graph = graph.road;

        Request req;
        Request::Step step;
        Road::VertexIterator vi, vi_end;
        bool found_origin = false;
        bool found_destination = false;

        for ( boost::tie( vi, vi_end ) = boost::vertices( road_graph ); vi != vi_end; vi++ ) {
            if ( road_graph[*vi].db_id == origin_id ) {
                req.origin = *vi;
                found_origin = true;
                break;
            }
        }

        if ( !found_origin ) {
            CERR << "Cannot find origin vertex ID " << origin_id << endl;
            return 1;
        }

        for ( boost::tie( vi, vi_end ) = boost::vertices( road_graph ); vi != vi_end; vi++ ) {
            if ( road_graph[*vi].db_id == destination_id ) {
                step.destination = *vi;
                found_destination = true;
                break;
            }
        }

        if ( !found_destination ) {
            CERR << "Cannot find destination vertex ID " << destination_id << endl;
            return 1;
        }

        req.steps.push_back( step );

        // the only optimizing criterion
        req.optimizing_criteria.push_back( CostDistance );

        COUT << endl << ">> pre_process" << endl;

        try {
            plugin->pre_process( req );
        }
        catch ( std::invalid_argument& e ) {
            CERR << "Can't process request : " << e.what() << std::endl;
            return 1;
        }

        COUT << endl << ">> process" << endl;
        plugin->process();
        COUT << endl << ">> post_process" << endl;
        plugin->post_process();

        COUT << endl << ">> result" << endl;
        plugin->result();

        COUT << endl << ">> cleanup" << endl;
        plugin->cleanup();

    }
    catch ( std::exception& e ) {
        CERR << "Exception: " << e.what() << endl;
    }

    return 0;
}
