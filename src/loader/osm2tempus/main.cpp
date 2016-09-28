#include "pgsql_writer.h"
#include "sqlite_writer.h"

#include "data_profile.h"

#include <boost/program_options.hpp>

void single_pass_pbf_read( const std::string& filename, Writer& writer, bool do_write_nodes = false, bool do_import_restrictions = true );
void two_pass_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions = true );
void two_pass_vector_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions = true );

namespace po = boost::program_options;

po::options_description opt_desc( "Allowed options" );

void usage()
{
    std::cout << "osm2routing allows to import routing data from an OSM PBF dump" << std::endl << std::endl;
    std::cout << opt_desc << std::endl;
}

int main(int argc, char** argv)
{
    using namespace std;

    string schema;
    string table;
    string pbf_file;
    string nodes_table;
    string restrictions_table;

    opt_desc.add_options()
        ( "help,h", "produce help message" )
        ( "input,i", po::value<string>(&pbf_file)->required(), "input OSM pbf file" )
        ( "pgis", po::value<string>()->default_value("dbname=tempus_test_db"), "PostGIS connection options" )
        ( "schema", po::value<string>(&schema)->default_value("tempus"), "set database schema" )
        ( "table", po::value<string>(&table)->default_value("road_section"), "set the table name to populate" )
        ( "sqlite", po::value<string>(), "SQLite output file" )
        ( "two-pass", "enable two pass reading" )
        ( "profile,p", po::value<string>()->default_value("tempus"), "use a data profile" )
        ( "list-profiles", "list available data profiles" )
        ( "keep-tags", "keep way tags when exporting" )
        ( "create-table,c", "create the table (and drop any existing one)" )
        ( "nodes-table", po::value<string>(&nodes_table), "write nodes to the given table" )
        ( "restrictions-table", po::value<string>(&restrictions_table)->default_value("road_restriction"), "write restrictions to the given table" )
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
                                               vm.count( "create-table" ),
                                               data_profile,
                                               keep_tags ) );
    }
    else if ( vm.count( "sqlite" ) ) {
        writer.reset( new SqliteWriter( vm["sqlite"].as<string>(), data_profile, keep_tags ) );
    }

    if ( vm.count( "two-pass" ) ) {
        two_pass_pbf_read( pbf_file, *writer );
    }
    else
    {
        single_pass_pbf_read( pbf_file, *writer, /*do_write_nodes = */ !nodes_table.empty() );
    }
    
    return 0;
}
