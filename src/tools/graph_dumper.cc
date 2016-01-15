#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include "routing_data.hh"
#include "routing_data_builder.hh"

#ifdef __linux__
#include <sys/resource.h>
#include <malloc.h>

// current heap usage (allocated minus freed), including overhead
static size_t heap_usage;
// total request size
static size_t heap_total_rsize;
// total allocated size, including malloc overhead
static size_t heap_total_size;

void* operator new(size_t s)
{
    void * p = malloc( s );
    size_t rs = *((size_t*)p - 1);
    heap_usage += rs;
    heap_total_rsize += s;
    heap_total_size += rs;
    return p;
}

void* operator new[](size_t s)
{
    void * p = malloc( s );
    size_t rs = *((size_t*)p - 1);
    heap_usage += rs;
    heap_total_rsize += s;
    heap_total_size += rs;
    return p;
}

void operator delete( void* p ) noexcept
{
    size_t rs = *((size_t*)p - 1);
    heap_usage -= rs;
    free(p);
}

void operator delete[]( void* p ) noexcept
{
    size_t rs = *((size_t*)p - 1);
    heap_usage -= rs;
    free(p);
}

struct MemInfo {
    size_t rss;
    size_t data;
};

MemInfo get_memory_usage()
{
    size_t dummy, rss, data;
    std::ifstream ifs( "/proc/self/statm" );
    ifs >> dummy >> rss >> dummy >> dummy >> dummy >> data;
    MemInfo info;
    info.rss = rss * 4 * 1024;
    info.data = data * 4 * 1024;
    return info;
}
#endif

int main( int argc, char* argv[] )
{ 
    namespace po = boost::program_options;
    using namespace std;
    using namespace Tempus;

    std::string db_options = "dbname=tempus_test_db";
    std::string schema = "tempus";
    std::string dump_file = "dump.bin";
    std::string graph_type = "multimodal_graph";

    po::options_description desc( "Allowed options" );
    desc.add_options()
    ( "help", "produce help message" )
    ( "list", "list the available graph types" )
    ( "db", po::value<string>(), "set database connection options" )
    ( "schema", po::value<string>(), "set database schema" )
    ( "graph", po::value<string>(), "set the graph type to import" )
    ( "dump_file", po::value<string>(), "set the name of the dump file to create" )
    ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );

    if ( vm.count( "help" ) ) {
        std::cout << desc << std::endl;
        return 1;
    }

    if ( vm.count( "list" ) ) {
        std::cout << "Available builders:" << std::endl;
        for ( const std::string& builder : RoutingDataBuilderRegistry::instance().builder_list() ) {
            std::cout << builder << std::endl;
        }
        return 1;
    }

    if ( vm.count( "db" ) ) {
        db_options = vm["db"].as<string>();
    }
    if ( vm.count( "schema" ) ) {
        schema = vm["schema"].as<string>();
    }
    if ( vm.count( "graph" ) ) {
        graph_type = vm["graph"].as<string>();
    }
    if ( vm.count( "dump_file" ) ) {
        dump_file = vm["dump_file"].as<string>();
    }

#ifdef __linux__
    MemInfo info;
    info = get_memory_usage();
    size_t real_size = info.rss /1024 /1024;
    size_t data_size = info.data /1024 /1024;
    std::cerr << "RSS: " << real_size << "MB" << std::endl;
    std::cerr << "Data: " << data_size << "MB" << std::endl;
    heap_usage = 0;
#endif

    {
        TextProgression progression;
        VariantMap options;
        options["db/options"] = Variant::from_string(db_options);
        options["db/schema"] = Variant::from_string(schema);
        const RoutingData* rd = load_routing_data( graph_type, progression, options );

#ifdef __linux__
        info = get_memory_usage();
        real_size = info.rss /1024 /1024;
        data_size = info.data /1024 /1024;
        std::cerr << "RSS: " << real_size << "MB" << std::endl;
        std::cerr << "Data: " << data_size << "MB" << std::endl;

        std::cerr << "Heap usage: " << (heap_usage/1024/1024) << "MB" << std::endl;
        std::cerr << "Total requested size: " << (heap_total_rsize/1024/1024) << "MB" << std::endl;
        std::cerr << "Total size: " << (heap_total_size/1024/1024) << "MB" << std::endl;

        float frag = (1.0 - float(heap_usage) / info.rss) * 100.0;
        std::cerr << "Fragmentation: " << frag << "%" << std::endl;
#endif
        
        std::cout << "dumping to " << dump_file << " ... " << std::endl;
        dump_routing_data( rd, dump_file, progression );
    }
}
