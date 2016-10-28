/**
 *   Copyright (C) 2016 Oslandia <infos@oslandia.com>
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

#include "pgsql_writer.h"
//#include "sqlite_writer.h"

#include "data_profile.h"

#include <boost/program_options.hpp>

void single_pass_pbf_read( const std::string& filename, Writer& writer, bool do_write_nodes = false, bool do_import_restrictions = true, size_t n_nodes = 0, size_t n_ways = 0 );
void two_pass_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions = true, size_t n_nodes = 0 );
void two_pass_vector_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions = true );
void pbf_stats( const std::string& filename );

namespace po = boost::program_options;

po::options_description opt_desc( "Allowed options" );

void usage()
{
    std::cout << "osm2routing allows to import routing data from an OSM PBF dump" << std::endl << std::endl;
    std::cout << opt_desc << std::endl;
}

void pbf_stats( const std::string& filename )
{
    off_t ways_offset = 0, relations_offset = 0;
    osm_pbf::osm_pbf_offsets<StdOutProgressor>( filename, ways_offset, relations_offset );
    std::cout << "Ways offset: " << std::dec << ways_offset << " (" << std::hex << ways_offset << ")" << std::endl;
    std::cout << "Relations offset: " << std::dec << relations_offset << " (" << std::hex << relations_offset << ")" << std::endl;

    std::cout << "Collecting statistics ..." << std::endl;
    size_t n_nodes, n_ways, n_highways, n_relations;
    osm_pbf::stats_osm_pbf<StdOutProgressor>( filename, n_nodes, n_ways, n_highways, n_relations );

    std::cout << "# nodes: " << std::dec << n_nodes << std::endl;
    std::cout << "# ways: " << n_ways << std::endl;
    std::cout << "# highways: " << n_highways << std::endl;
    std::cout << "# relations: " << n_relations << std::endl;
}

int main(int argc, char** argv)
{
    using namespace std;

    string schema;
    string table;
    string pbf_file;
    string nodes_table;
    string restrictions_table;
    size_t n_nodes = 0;
    size_t n_ways = 0;

    opt_desc.add_options()
        ( "help,h", "produce help message" )
        ( "input,i", po::value<string>(&pbf_file)->required(), "input OSM pbf file" )
        ( "pgis", po::value<string>()->default_value("dbname=tempus_test_db"), "PostGIS connection options" )
        ( "schema", po::value<string>(&schema)->default_value("tempus"), "set database schema" )
        ( "table", po::value<string>(&table)->default_value("road_section"), "set the table name to populate" )
        /*( "sqlite", po::value<string>(), "SQLite output file" )*/
        ( "two-pass", "enable two pass reading" )
        ( "profile,p", po::value<string>()->default_value("tempus"), "use a data profile" )
        ( "list-profiles", "list available data profiles" )
        ( "keep-tags", "keep way tags when exporting" )
        ( "create,c", "create tables (and drop any existing one)" )
        ( "truncate,t", "truncate tables" )
        ( "nodes-table", po::value<string>(&nodes_table)->default_value("road_node"), "write nodes to the given table" )
        ( "restrictions-table", po::value<string>(&restrictions_table)->default_value("road_restriction"), "write restrictions to the given table" )
        ( "n-nodes", po::value<size_t>(&n_nodes), "give a hint about the number of nodes the input file contains" )
        ( "n-ways", po::value<size_t>(&n_ways), "give a hint about the number of ways the input file contains" )
        ( "planet", "use a huge vector to cache nodes (need ~50GB of RAM)" )
        ( "stats", "only count the number of nodes, ways and relations in the input file" )
    ;

    po::variables_map vm;
    try
    {
        po::store( po::parse_command_line( argc, argv, opt_desc ), vm );
        po::notify( vm );
    }
    catch ( po::error& e )
    {
        std::cerr << "Argument error: " << e.what() << std::endl << std::endl;
        std::cerr << opt_desc << std::endl;
        return 1;
    }

    if ( vm.count( "help" ) ) {
        usage();
        return 1;
    }

    if ( vm.count( "list-profiles" ) ) {
        for ( const auto& p: g_data_profile_registry ) {
            std::cout << p.first << std::endl;
        }
        return 0;
    }

    if ( pbf_file.empty() ) {
        std::cerr << "An input PBF file must be specified" << std::endl;
        return 1;
    }

    if ( vm.count("stats") ) {
        pbf_stats( pbf_file );
        return 0;
    }

    DataProfile* data_profile = nullptr;
    if ( vm.count( "profile" ) ) {
        string profile = vm["profile"].as<string>();
        auto pit = g_data_profile_registry.find( profile );
        if ( pit == g_data_profile_registry.end() ) {
            std::cerr << "unknown profile " << profile << std::endl;
            return 1;
        }
        data_profile = pit->second;
    }

    bool keep_tags = vm.count( "keep-tags" );
    std::unique_ptr<Writer> writer;
    if ( vm.count( "pgis" ) ) {
        writer.reset( new SQLBinaryCopyWriter( vm["pgis"].as<string>(),
                                               schema,
                                               table,
                                               nodes_table,
                                               restrictions_table,
                                               vm.count( "create" ),
                                               vm.count( "truncate" ),
                                               data_profile,
                                               keep_tags ) );
    }
    /*else if ( vm.count( "sqlite" ) ) {
        writer.reset( new SqliteWriter( vm["sqlite"].as<string>(), data_profile, keep_tags ) );
    }
    */

    if ( vm.count( "planet" ) ) {
        two_pass_vector_pbf_read( pbf_file, *writer, /*do_import_restrictions = */ true );
    }
    else if ( vm.count( "two-pass" ) ) {
        two_pass_pbf_read( pbf_file, *writer, /*do_import_restrictions = */ true, n_nodes );
    }
    else
    {
        single_pass_pbf_read( pbf_file, *writer, /*do_write_nodes = */ !nodes_table.empty(), /*do_import_restrictions = */ true, n_nodes, n_ways );
    }
    
    return 0;
}
