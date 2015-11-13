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
#include "plugin_factory.hh"

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

    VariantMap options;
    options["db/options"] = Variant::fromString( db_options );

    ///
    /// Plugins
    try {
        TextProgression progression;
        Plugin* plugin = PluginFactory::instance()->create_plugin( plugin_name, progression, options );

        COUT << "[plugin " << plugin->name() << "]" << endl;

        //
        // Build the user request
        const Multimodal::Graph* graph = dynamic_cast<const Multimodal::Graph*>( plugin->routing_data() );
        if ( graph == nullptr ) {
            CERR << "Cannot cast to multimodal graph" << std::endl;
            return 1;
        }
        Request req;

        req.set_origin( origin_id );
        req.set_destination( destination_id );

        std::unique_ptr<PluginRequest> plugin_request( plugin->request() );
        std::unique_ptr<Result> result( plugin_request->process( req ) );
    }
    catch ( std::exception& e ) {
        CERR << "Exception: " << e.what() << endl;
    }

    return 0;
}
