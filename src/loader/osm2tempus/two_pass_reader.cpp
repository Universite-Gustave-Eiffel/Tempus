#include "osm2tempus.h"
#include "writer.h"
#include "section_splitter.h"

#include <unordered_set>

template <typename PointCacheType>
struct PbfReaderPass1
{
public:
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_.insert( osmid, Point(lon, lat) );
    }

    void way_callback( uint64_t osmid, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        for ( uint64_t node: nodes ) {
            points_.inc_uses( node );
        }
    }
    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    PointCacheType& points() { return points_; }
private:
    PointCacheType points_;
};

template <typename PointCacheType>
struct PbfReaderPass2
{
    PbfReaderPass2( PointCacheType& points, Writer& writer ):
        points_( points ), writer_( writer ), section_splitter_( points_ )
    {
        writer_.begin_sections();
    }
    
    void node_callback( uint64_t /*osmid*/, double /*lon*/, double /*lat*/, const osm_pbf::Tags &/*tags*/ )
    {
    }

    void way_callback( uint64_t way_id, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        for ( uint64_t node: nodes ) {
            // ignore ways with unknown nodes
            if ( !points_.find( node ) )
                return;
        }

        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = nodes[0];
        uint64_t node_from;
        std::vector<uint64_t> section_nodes;
        for ( size_t i = 1; i < nodes.size(); i ++ ) {
            uint64_t node = nodes[i];

            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == nodes.size() - 1 || points_.uses( node ) > 1 ) {
                //split_into_sections( way_id, node_from, node, section_nodes, tags );
                section_splitter_( way_id, node_from, node, section_nodes, tags,
                                   [&](uint64_t lway_id, uint64_t lsection_id, uint64_t lnode_from, uint64_t lnode_to, const std::vector<Point>& lpts, const osm_pbf::Tags& ltags)
                                   {
                                       writer_.write_section( lway_id, lsection_id, lnode_from, lnode_to, lpts, ltags );
                                   });
                section_start = true;
            }
            old_node = node;
        }
    }

    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    ~PbfReaderPass2()
    {
        writer_.end_sections();
    }

private:
    PointCacheType& points_;

    Writer& writer_;

    SectionSplitter<PointCacheType> section_splitter_;
};

template <typename PointCacheType>
void two_pass_pbf_read_( const std::string& filename, Writer& writer )
{
    off_t ways_offset = 0, relations_offset = 0;
    osm_pbf::osm_pbf_offsets<StdOutProgressor>( filename, ways_offset, relations_offset );
    std::cout << "Ways offset: " << ways_offset << std::endl;
    std::cout << "Relations offset: " << relations_offset << std::endl;

    std::cout << "first pass" << std::endl;
    PbfReaderPass1<PointCacheType> p1;
    osm_pbf::read_osm_pbf<PbfReaderPass1<PointCacheType>, StdOutProgressor>( filename, p1, 0, relations_offset );
    std::cout << p1.points().size() << " nodes cached" << std::endl;
    std::cout << "second pass" << std::endl;
    PbfReaderPass2<PointCacheType> p2( p1.points(), writer );
    osm_pbf::read_osm_pbf<PbfReaderPass2<PointCacheType>, StdOutProgressor>( filename, p2, ways_offset, relations_offset );
}

void two_pass_pbf_read( const std::string& filename, Writer& writer )
{
    two_pass_pbf_read_<PointCache>( filename, writer );
}

void two_pass_vector_pbf_read( const std::string& filename, Writer& writer )
{
    two_pass_pbf_read_<PointCacheVector>( filename, writer );
}








