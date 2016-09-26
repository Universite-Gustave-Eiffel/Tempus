#include "osm2tempus.h"
#include "writer.h"
#include "section_splitter.h"

#include <unordered_set>

class PointCacheVector
{
public:
    using CacheType = std::vector<Point>;
    PointCacheVector()
    {
        points_.resize( max_node_id_ );
    }

    CacheType::const_iterator begin() const
    {
        return points_.begin();
    }
    CacheType::const_iterator end() const
    {
        return points_.end();
    }
    CacheType::iterator begin()
    {
        return points_.begin();
    }
    CacheType::iterator end()
    {
        return points_.end();
    }
    CacheType::const_iterator find( uint64_t id ) const
    {
        return points_.begin() + id;
    }
    CacheType::iterator find( uint64_t id )
    {
        return points_.begin() + id;
    }

    size_t size() const
    {
        return points_.size();
    }

    const Point& at( uint64_t id ) const
    {
        return points_[id];
    }
    Point& at( uint64_t id )
    {
        return points_[id];
    }

    /// Insert a point with a given id
    void insert( uint64_t id, Point&& point )
    {
        points_[id] = point;
    }

    /// Insert a point with a given id
    void insert( uint64_t id, const Point& point )
    {
        points_[id] = point;
    }

    /// Insert a new point and return the new id
    uint64_t insert( const Point& point )
    {
        uint64_t ret = last_node_id_;
        points_[last_node_id_--] = point;
        return ret;
    }
private:
    CacheType points_;
    // node ids that are introduced to split multi edges
    // we count them backward from 2^64 - 1
    // this should not overlap current OSM node ID (~ 2^32 in july 2016)
    const uint64_t max_node_id_ = 5000000000LL;
    uint64_t last_node_id_ = 5000000000LL;
};

struct PbfReaderPass1Vector
{
public:
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_.insert( osmid, Point(lon, lat) );
    }

    void way_callback( uint64_t /*osmid*/, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        for ( uint64_t node: nodes ) {
            Point& p = points_.at( node );
            int uses = p.uses();
            if ( uses < 2 )
                p.set_uses( uses + 1 );
        }
    }
    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    PointCacheVector& points() { return points_; }
private:
    PointCacheVector points_;
};

struct PbfReaderPass2Vector
{
    PbfReaderPass2Vector( PointCacheVector& points, Writer& writer ):
        points_( points ), writer_( writer ), section_splitter_( points_ )
    {}
    
    void node_callback( uint64_t /*osmid*/, double /*lon*/, double /*lat*/, const osm_pbf::Tags &/*tags*/ )
    {
    }

    void way_callback( uint64_t way_id, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = nodes[0];
        uint64_t node_from;
        std::vector<uint64_t> section_nodes;
        for ( size_t i = 1; i < nodes.size(); i ++ ) {
            uint64_t node = nodes[i];
            const Point& pt = points_.at( node );
            if ( pt.uses() == 0 ) {
                // ignore ways with unknown nodes
                section_start = true;
                continue;
            }

            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == nodes.size() - 1 || pt.uses() > 1 ) {
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

private:
    PointCacheVector& points_;

    Writer& writer_;

    SectionSplitter<PointCacheVector> section_splitter_;
};

void two_pass_vector_pbf_read( const std::string& filename, Writer& writer )
{
    std::cout << "first pass" << std::endl;
    PbfReaderPass1Vector p1;
    osm_pbf::read_osm_pbf<PbfReaderPass1Vector, StdOutProgressor>( filename, p1 );
    std::cout << p1.points().size() << " nodes cached" << std::endl;
    std::cout << "second pass" << std::endl;
    PbfReaderPass2Vector p2( p1.points(), writer );
    osm_pbf::read_osm_pbf<PbfReaderPass2Vector, StdOutProgressor>( filename, p2 );
}







