#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <iomanip>
#include <functional> // hash
#include <boost/program_options.hpp>


#include "db.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "osmpbfreader.h"
#pragma GCC diagnostic pop

// a pair of nodes
using node_pair = std::pair<uint64_t, uint64_t>;

namespace std {
  template <> struct hash<node_pair>
  {
    size_t operator()(const node_pair & np) const
    {
        // cf http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
        size_t h1 = hash<uint64_t>()( np.first );
        size_t h2 = hash<uint64_t>()( np.second );
        return h2 + 0x9e3779b9 + (h1<<6) + (h1>>2);
    }
  };
}

namespace osm_pbf = CanalTP;

struct Point
{
    Point() {}
    Point( double mlon, double mlat ) : lon( mlon ), lat( mlat ) {}
    double lon, lat;
    int uses = 0;
};

struct Way
{
    std::vector<uint64_t> nodes;
    osm_pbf::Tags tags;
    bool ignored = false;
};

class Writer
{
public:
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) = 0;
};

std::string escape_pgquotes( std::string s )
{
    size_t pos = 0;
    while ((pos = s.find('\'', pos)) != std::string::npos) {
         s.replace(pos, 1, "''");
         pos += 2;
    }
    return s;
}

/// TODO: turn virtual into template-based

///
/// A basic text-based SQL writer
class SQLWriter : public Writer
{
public:
    SQLWriter() : section_id( 0 )
    {
        //std::cout << "set client encoding to 'utf8';" << std::endl;
        std::cout << "drop table if exists edges;" << std::endl;
        std::cout << "create table edges(id serial primary key, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326));" << std::endl;
        std::cout << "begin;" << std::endl;
    }
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
    {
        std::cout << "INSERT INTO edges (id,node_from,node_to, geom, tags) VALUES (" << ++section_id << ","
                  << node_from << ","
                  << node_to << ","
                  << "'SRID=4326;LINESTRING(";
        for ( size_t i = 0; i < points.size(); i++ )
        {
            std::cout << std::setprecision(8) << points[i].lon << " " << points[i].lat;
            if ( i < points.size() - 1 )
                std::cout << ",";
        }
        std::cout << ")',";
        std::cout << "hstore(array[";
        bool first = true;
        for ( const auto& p: tags ) {
            if ( !first ) {
                std::cout << ",";
            }
            first = false;
            std::cout << "'" << p.first << "','" << escape_pgquotes(p.second) << "'";
        }
        std::cout << "]));" << std::endl;
    }

    ~SQLWriter()
    {
        std::cout << "commit;" << std::endl;
    }

private:
    uint64_t section_id;
};

std::string linestring_to_ewkb( const std::vector<Point>& points )
{
    std::string ewkb;
    ewkb.resize( 1 + 4 /*type*/ + 4 /*srid*/ + 4 /*size*/ + 8 * points.size() * 2 );
    memcpy( &ewkb[0], "\x01"
            "\x02\x00\x00\x20"  // linestring + has srid
            "\xe6\x10\x00\x00", // srid = 4326
            9
            );
    // size
    *reinterpret_cast<uint32_t*>( &ewkb[9] ) = points.size();
    for ( size_t i = 0; i < points.size(); i++ ) {
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 0 ) = points[i].lon;
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 8 ) = points[i].lat;
    }
    return ewkb;
}

std::string binary_to_hextext( const std::string& str )
{
    std::ostringstream ostr;
    for ( char c : str ) {
        ostr << std::hex << std::setw( 2 ) << std::setfill('0') << static_cast<int>( static_cast<unsigned char>( c ) );
    }
    return ostr.str();
}

std::string tags_to_binary( const osm_pbf::Tags& tags )
{
    uint32_t size1, size2, wsize;
    // total size;
    uint32_t tsize = 4;
    for ( const auto& tag: tags ) {
        tsize += tag.first.length() + tag.second.length() + 8;
    }
    
    std::string out;
    out.resize( tsize + 4 );
    char *p = &out[0];

    // total size
    wsize = htonl( tsize );
    memcpy( p, &wsize, 4 ); p += 4;
    
    // number of tags
    wsize = htonl( tags.size() );
    memcpy( p, &wsize, 4 ); p += 4;
    for ( const auto& tag : tags )
    {
        size1 = tag.first.length();
        size2 = tag.second.length();
        wsize = htonl( size1 );
        memcpy( p, &wsize, 4 ); p += 4;
        memcpy( p, tag.first.data(), size1 ); p += size1;
        wsize = htonl( size2 );
        memcpy( p, &wsize, 4 ); p += 4;
        memcpy( p, tag.second.data(), size2 ); p += size2;
    }
    return out;
}

///
/// A basic COPY-based SQL writer
class SQLCopyWriter : public Writer
{
public:
    SQLCopyWriter( const std::string& db_params ) : section_id( 0 ), db( db_params )
    {
        db.exec( "drop table if exists edges" );
        db.exec( "create unlogged table edges(id serial primary key, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326))" );
        db.exec( "copy edges(node_from, node_to, geom) from stdin with delimiter ';'" );
    }
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ )
    {
        std::ostringstream data;
        data << node_from << ";" << node_to << ";" << binary_to_hextext( linestring_to_ewkb( points ) ) << std::endl;
        db.put_copy_data( data.str() );
    }

    ~SQLCopyWriter()
    {
        db.put_copy_end();
    }

private:
    uint64_t section_id;
    Db::Connection db;
};

///
/// A binary COPY-based SQL writer
class SQLBinaryCopyWriter : public Writer
{
public:
    SQLBinaryCopyWriter( const std::string& db_params ) : section_id( 0 ), db( db_params )
    {
        db.exec( "drop table if exists edges" );
        db.exec( "create unlogged table edges(id serial primary key, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326))" );
        db.exec( "copy edges(node_from, node_to, tags, geom) from stdin with (format binary)" );
        const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0";
        db.put_copy_data( header, 19 );
    }
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
    {
        const size_t l = 2 /*size*/ + (4+8)*2;
        char data[ l ];
        // number of fields
        uint16_t size = htons( 4 );
        // size of a uint64
        uint32_t size2 = htonl( 8 );
        char *p = &data[0];
        memcpy( p, &size, 2 ); p += 2;
        memcpy( p, &size2, 4 ); p+= 4;
        node_from = htobe64( node_from );
        node_to = htobe64( node_to );
        memcpy( p, &node_from, 8 ); p+= 8;
        memcpy( p, &size2, 4 ); p+= 4;
        memcpy( p, &node_to, 8 ); p+= 8;
        db.put_copy_data( data, l );

        db.put_copy_data( tags_to_binary( tags ) );

        // geometry
        std::string geom_wkb = linestring_to_ewkb( points );
        uint32_t geom_size = htonl( geom_wkb.length() );
        db.put_copy_data( reinterpret_cast<char*>( &geom_size ), 4 );
        db.put_copy_data( geom_wkb );
    }

    ~SQLBinaryCopyWriter()
    {
        std::string end = "\xff\xff";
        db.put_copy_data( end );
        db.put_copy_end();
    }

private:
    uint64_t section_id;
    Db::Connection db;
};

struct PbfReader
{
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points[osmid] = Point(lon, lat);
    }

    void way_callback( uint64_t osmid, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        auto r = ways.emplace( osmid, Way() );
        Way& w = r.first->second;
        w.tags = tags;
        w.nodes = nodes;
    }

    void mark_points_and_ways()
    {
        for ( auto way_it = ways.begin(); way_it != ways.end(); way_it++ ) {
            // mark each nodes as being used
            for ( uint64_t node: way_it->second.nodes ) {
                auto it = points.find( node );
                if ( it != points.end() ) {
                    it->second.uses++;
                }
                else {
                    // unknown point
                    way_it->second.ignored = true;
                }
            }
        }
    }

    ///
    /// Convert raw OSM ways to road sections. Sections are road parts between two intersections.
    void write_sections( Writer& writer )
    {
        for ( auto way_it = ways.begin(); way_it != ways.end(); way_it++ ) {
            const Way& way = way_it->second;
            if ( way.ignored )
                continue;

            way_to_sections( way, writer );
        }
    }

    void way_to_sections( const Way& way, Writer& writer )
    {
        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = way.nodes[0];
        uint64_t node_from;
        std::vector<uint64_t> section_nodes;
        //Point old_pt = points.find( old_node )->second;
        for ( size_t i = 1; i < way.nodes.size(); i++ ) {
            uint64_t node = way.nodes[i];
            const Point& pt = points.find( node )->second;
            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == way.nodes.size() - 1 || pt.uses > 1 ) {
                split_into_sections( node_from, node, section_nodes, way.tags, writer );
                //writer.write_section( node_from, node, section_pts, way.tags );
                section_start = true;
            }
            old_node = node;
        }
    }

    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

private:
    std::unordered_map<uint64_t, Point> points;
    std::unordered_map<uint64_t, Way> ways;

    // structure used to detect multi edges
    std::unordered_set<node_pair> way_node_pairs;
    //std::unordered_map<node_pair, uint64_t> way_node_pairs;
    uint64_t way_id = 0;

    // node ids that are introduced to split multi edges
    // we count them backward from 2^64 - 1
    // this should not overlap current OSM node ID (~ 2^32 in july 2016)
    uint64_t last_artificial_node_id = 0xFFFFFFFFFFFFFFFFLL;

        ///
    /// Check if a section with the same (from, to) pair exists
    /// In which case, it is split into two sections
    /// in order to avoid multigraphs
    void split_into_sections( uint64_t node_from, uint64_t node_to, const std::vector<uint64_t>& nodes, const osm_pbf::Tags& tags, Writer& writer )
    {
        // in order to avoid multigraphs
        node_pair p ( node_from, node_to );
        if ( way_node_pairs.find( p ) != way_node_pairs.end() ) {
            // split the way
            // if there are more than two nodes, just split on a node
            std::vector<Point> before_pts, after_pts;
            uint64_t center_node;
            if ( nodes.size() > 2 ) {
                size_t center = nodes.size() / 2;
                center_node = nodes[center];
                for ( size_t i = 0; i <= center; i++ ) {
                    before_pts.push_back( points.find( nodes[i] )->second );
                }
                for ( size_t i = center; i < nodes.size(); i++ ) {
                    after_pts.push_back( points.find( nodes[i] )->second );
                }
            }
            else {
                const Point& p1 = points.find( nodes[0] )->second;
                const Point& p2 = points.find( nodes[1] )->second;
                Point center_point( ( p1.lon + p2.lon ) / 2.0, ( p1.lat + p2.lat ) / 2.0 );
                
                before_pts.push_back( points.find( nodes[0] )->second );
                before_pts.push_back( center_point );
                after_pts.push_back( center_point );
                after_pts.push_back( points.find( nodes[1] )->second );

                // add a new point
                center_node = last_artificial_node_id;
                points[last_artificial_node_id--] = center_point;
            }
            writer.write_section( node_from, center_node, before_pts, tags );
            writer.write_section( center_node, node_to, after_pts, tags );
        }
        else {
            way_node_pairs.insert( p );
            std::vector<Point> section_pts;
            for ( uint64_t node: nodes ) {
                section_pts.push_back( points.find( node )->second );
            }
            writer.write_section( node_from, node_to, section_pts, tags );
        }
    }
};

int main(int argc, char** argv)
{
    namespace po = boost::program_options;
    using namespace std;

    string db_options = "dbname=tempus_test_db";
    string schema = "_tempus_import";
    string table = "highway";
    string pbf_file = "";

    po::options_description desc( "Allowed options" );
    desc.add_options()
    ( "help", "produce help message" )
    ( "db", po::value<string>(), "set database connection options" )
    ( "schema", po::value<string>(), "set database schema" )
    ( "table", po::value<string>(), "set the table name to populate" )
    ( "pbf", po::value<string>(), "input OSM pbf file" )
    ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );

    if ( vm.count( "help" ) ) {
        std::cout << desc << std::endl;
        return 1;
    }

    if ( vm.count( "db" ) ) {
        db_options = vm["db"].as<string>();
    }
    if ( vm.count( "schema" ) ) {
        schema = vm["schema"].as<string>();
    }
    if ( vm.count( "table" ) ) {
        table = vm["table"].as<string>();
    }
    if ( vm.count( "pbf" ) ) {
        pbf_file = vm["pbf"].as<string>();
    }

    if ( pbf_file.empty() ) {
        std::cerr << "An input PBF file must be specified" << std::endl;
        return 1;
    }

     PbfReader p;
     osm_pbf::read_osm_pbf(pbf_file, p);
     p.mark_points_and_ways();
     SQLBinaryCopyWriter w( db_options );
     p.write_sections( w );

     return 0;
}
