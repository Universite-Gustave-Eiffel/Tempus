#ifndef OSM2TEMPUS_SECTION_SPLITTER
#define OSM2TEMPUS_SECTION_SPLITTER

#include "osm2tempus.h"

#include <unordered_set>

template <typename PointCacheType>
// requirements on PointCacheType
// - const Point& PointCacheType::at( node_id )
// - uint64_t     PointCacheType::insert( const Point& ) -> new id
class SectionSplitter
{
public:
    SectionSplitter( PointCacheType& points )
        : points_( points ),
          section_id_( 0 )
    {}

    template <typename SectionFoo>
    void operator()( uint64_t way_id,
                     uint64_t node_from,
                     uint64_t node_to,
                     const std::vector<uint64_t>& nodes,
                     const osm_pbf::Tags& tags,
                     SectionFoo emit_section )
    {
        node_pair p ( node_from, node_to );
        if ( way_node_pairs_.find( p ) != way_node_pairs_.end() ) {
            // split the way
           // if there are more than two nodes, just split on a node
            std::vector<Point> before_pts, after_pts;
            uint64_t center_node;
            if ( nodes.size() > 2 ) {
                size_t center = nodes.size() / 2;
                center_node = nodes[center];
                for ( size_t i = 0; i <= center; i++ ) {
                    before_pts.push_back( points_.at( nodes[i] ) );
                }
                for ( size_t i = center; i < nodes.size(); i++ ) {
                    after_pts.push_back( points_.at( nodes[i] ) );
                }
            }
            else {
                const typename PointCacheType::PointType& p1 = points_.at( nodes[0] );
                const typename PointCacheType::PointType& p2 = points_.at( nodes[1] );
                Point center_point( ( p1.lon() + p2.lon() ) / 2.0, ( p1.lat() + p2.lat() ) / 2.0 );
                
                before_pts.push_back( points_.at( nodes[0] ) );
                before_pts.push_back( center_point );
                after_pts.push_back( center_point );
                after_pts.push_back( points_.at( nodes[1] ) );

                // add a new point
                center_node = points_.insert( center_point );
            }
            //writer_.write_section( way_id, ++section_id_, node_from, center_node, before_pts, tags );
            //writer_.write_section( way_id, ++section_id_, center_node, node_to, after_pts, tags );
            emit_section( way_id, ++section_id_, node_from, center_node, before_pts, tags );
            emit_section( way_id, ++section_id_, center_node, node_to, after_pts, tags );
        }
        else {
            way_node_pairs_.insert( p );
            std::vector<Point> section_pts;
            for ( uint64_t node: nodes ) {
                section_pts.push_back( points_.at( node ) );
            }
            //writer_.write_section( way_id, ++section_id_, node_from, node_to, section_pts, tags );
            emit_section( way_id, ++section_id_, node_from, node_to, section_pts, tags );
        }
    }
 private:
    // reference to the point cache
    PointCacheType& points_;
    // structure used to detect multi edges
    std::unordered_set<node_pair> way_node_pairs_;
    // the current section_id
    uint64_t section_id_;
};

#endif
