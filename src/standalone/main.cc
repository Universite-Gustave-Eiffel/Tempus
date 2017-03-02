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
#include <vector>
#include <iterator>
#include <sstream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "plugin_factory.hh"
#include "transport_modes.hh"

using namespace std;
using namespace Tempus;

namespace po = boost::program_options;

DateTime to_date_time( const std::string& s )
{
    DateTime dt;
    istringstream iss( s );
    auto facet = new boost::posix_time::time_input_facet();
    facet->set_iso_extended_format();
    iss.imbue(std::locale(iss.getloc(), facet));
    if ( ! ( iss >> dt ) ) {
        throw std::runtime_error( "Can't parse date time in " + s );
    }
    return dt;
}

int main_( int argc, char* argv[] )
{
    // default db options
    string db_options = "dbname=tempus_test_db";
    string plugin_name = "sample_road_plugin";
    Tempus::db_id_t origin_id = 21406;
    Tempus::db_id_t destination_id = 21015;
    int repeat = 1;
    bool pvad = true;
    Tempus::CostId opt_criterion = CostId::CostDuration;

    // parse command line arguments
    po::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help,h", "produce help message" )
        ( "db,d", po::value<string>(), "set database connection options" )
        ( "plugin,l", po::value<string>(), "set the plugin name to launch" )
        ( "origin", po::value<Tempus::db_id_t>(), "set the origin vertex id" )
        ( "destination", po::value<Tempus::db_id_t>(), "set the destination vertex id" )
        ( "modes", po::value<std::vector<int>>()->multitoken(), "set the allowed modes (space separated)" )
        ( "pvad", po::value<bool>(), "set 'private vehicule at destination'" )
        ( "depart-after", po::value<string>(), "set the time constraint to depart after this date & time" )
        ( "arrive-before", po::value<string>(), "set the time constraint to arrive before this date & time" )
        ( "optimize-distance", po::value<bool>(), "optimize distance" )
        ( "options", po::value<std::vector<string>>()->multitoken(), "set the plugin options option:type=value (space separated)" )
        ( "repeat,n", po::value<int>(), "set the repeat count (for profiling)" )
        ( "load-from,L", po::value<string>(), "set the dump file to load")
        ( "no-catch", "Debug mode, don't catch exceptions" )
        ;

    po::variables_map vm;
    try {
        po::store( po::parse_command_line( argc, argv, desc ), vm );
    }
    catch ( po::unknown_option& e ) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    po::notify( vm );

    if ( vm.count( "help" ) ) {
        COUT << desc << "\n";
        return 1;
    }

    tempus_init();

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

    if ( vm.count( "repeat" ) ) {
        repeat = vm["repeat"].as<int>();
    }

    if ( vm.count( "pvad" ) ) {
        pvad = vm["pvad"].as<bool>();
    }

    if ( vm.count( "optimize-distance" ) ) {
        opt_criterion = CostId::CostDistance;
    }

    vector<int> modes;
    if ( vm.count( "modes" ) ) {
        modes = vm["modes"].as<std::vector<int>>();
    }

    Request::TimeConstraint tc;
    tc.set_type( Request::TimeConstraint::NoConstraint );
    if ( vm.count( "depart-after" ) ) {
        string s = vm["depart-after"].as<string>();
        tc.set_date_time( to_date_time( s ) );
        tc.set_type( Request::TimeConstraint::ConstraintAfter );
    }
    if ( vm.count( "arrive-before" ) ) {
        string s = vm["arrive-before"].as<string>();
        tc.set_date_time( to_date_time( s ) );
        tc.set_type( Request::TimeConstraint::ConstraintBefore );
    }

    VariantMap options;
    options["db/options"] = Variant::from_string( db_options );

    if ( vm.count( "options" ) ) {
        vector<string> opts = vm["options"].as<std::vector<string>>();
        for ( const string& o : opts ) {
            string value;
            string option;
            string type;
            size_t p = o.find( '=' );
            if ( p != string::npos ) {
                string l = o.substr( 0, p );
                size_t p2 = l.find( ':' );
                value = o.substr( p + 1 );
                if ( p2 != string::npos ) {
                    option = l.substr( 0, p2 );
                    type = l.substr( p2 + 1 );
                }
            }
            if ( type == "bool" ) {
                options[option] = Variant::from_bool( value == "true" || value == "1" );
            }
            else if ( type == "int" ) {
                options[option] = Variant::from_int( lexical_cast<int>( value ) );
            }
            else if ( type == "float" ) {
                options[option] = Variant::from_float( lexical_cast<float>( value ) );
            }
            else if ( type == "str" ) {
                options[option] = Variant::from_string( value );
            }
        }
    }

    if ( vm.count("load-from") ) {
        options["from_file"] = Variant::from_string( vm["load-from"].as<string>() );
    }

    for ( auto p : options ) {
        cout << p.first << "=" << p.second.str() << endl;
    }

    ///
    /// Plugins
    TextProgression progression;
    Plugin* plugin = PluginFactory::instance()->create_plugin( plugin_name, progression, options );

    COUT << "[plugin " << plugin->name() << "]" << endl;

    Request req;

    req.add_criterion( opt_criterion );
    req.set_origin( origin_id );
    Request::Step dest;
    dest.set_location( destination_id );
    dest.set_private_vehicule_at_destination( pvad );
    dest.set_constraint( tc );
    req.set_destination( dest );
    if ( modes.empty() )
        modes.push_back( TransportModeWalking );
    for ( int m : modes ) {
        req.add_allowed_mode( m );
    }

    double avg_time = 0.0;
    for ( int i = 0; i < repeat; i++ ) {
        std::unique_ptr<PluginRequest> plugin_request( plugin->request( options ) );
        std::unique_ptr<Result> result( plugin_request->process( req ) );
        for (auto r: *result) {
            std::cout << r.is_roadmap() << std::endl;
        }
        double t = plugin_request->metrics()["time_s"].as<double>() * 1000;
        std::cout << "Time: " << t << "ms" << " iterations: " << plugin_request->metrics()["iterations"].as<int64_t>() << std::endl;
        avg_time += t;
    }
    avg_time /= repeat;
    std::cout << "Average time: " << avg_time << "ms" << std::endl;

    return 0;
}

int main(int argc, char* argv[])
{
    const std::vector<std::string> args{ argv, argv + argc };
    bool debug_mode = find( begin(args), end(args), "--no-catch" ) != end(args);

    if ( debug_mode ) {
        return main_( argc, argv );
    }
    else {
	try
	{
            return main_(argc, argv);
	}
	catch (std::exception& e)
	{
            std::cerr << "Exception: " << e.what() << std::endl;
	}
    }
    return 1;
}

