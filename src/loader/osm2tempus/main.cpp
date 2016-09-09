#include <unordered_map>
#include <iostream>
#include <iomanip>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "osmpbfreader.h"
#pragma GCC diagnostic pop

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
        w.nodes = nodes;
        w.tags = tags;
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
            //uint64_t osmid = way_it.first;

            way_to_sections( way, writer );
        }
    }

    void way_to_sections( const Way& way, Writer& writer )
    {
        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = way.nodes[0];
        uint64_t node_from;
        std::vector<Point> section_pts;
        Point old_pt = points.find( old_node )->second;
        for ( size_t i = 1; i < way.nodes.size(); i++ ) {
            uint64_t node = way.nodes[i];
            const Point& pt = points.find( node )->second;
            if ( section_start ) {
                section_pts.clear();
                section_pts.push_back( old_pt );
                node_from = old_node;
                section_start = false;
            }
            section_pts.push_back( pt );
            if ( i == way.nodes.size() - 1 || pt.uses > 1 ) {
                writer.write_section( node_from, node, section_pts, way.tags );
                section_start = true;
            }
            old_pt = pt;
            old_node = node;
        }
    }

    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    std::unordered_map<uint64_t, Point> points;
    std::unordered_map<uint64_t, Way> ways;
    uint64_t way_id = 0;
};

int main(int argc, char** argv)
{
     if(argc != 2) {
         std::cout << "Usage: " << argv[0] << " file_to_read.osm.pbf" << std::endl;
         return 1;
     }

     PbfReader p;
     osm_pbf::read_osm_pbf(argv[1], p);
     p.mark_points_and_ways();
     SQLWriter w;
     p.write_sections( w );

     return 0;
}
