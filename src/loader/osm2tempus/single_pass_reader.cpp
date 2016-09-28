#include <unordered_set>
#include <set>

#include "osm2tempus.h"
#include "writer.h"
#include "section_splitter.h"
#include "restrictions.h"

struct Way
{
    std::vector<uint64_t> nodes;
    osm_pbf::Tags tags;
    bool ignored = false;
};


using WayCache = std::unordered_map<uint64_t, Way>;

template <bool do_import_restrictions_ = false>
struct PbfReader
{
    PbfReader( RestrictionReader* restrictions = 0 ) :
        restrictions_( restrictions ),
        section_splitter_( points_ )
    {}
    
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_.insert( osmid, PointCache::PointType(lon, lat) );
    }

    void way_callback( uint64_t osmid, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        auto r = ways_.emplace( osmid, Way() );
        Way& w = r.first->second;
        w.tags = tags;
        w.nodes = nodes;
    }

    template <typename Progressor>
    void mark_points_and_ways( Progressor& progressor )
    {
        progressor( 0, ways_.size() );
        size_t i = 0;
        for ( auto way_it = ways_.begin(); way_it != ways_.end(); way_it++ ) {
            // mark each nodes as being used
            for ( uint64_t node: way_it->second.nodes ) {
                auto pt = points_.find( node );
                if ( pt ) {
                    if ( pt->uses() < 2 )
                        pt->set_uses( pt->uses() + 1 );
                }
                else {
                    // unknown point
                    way_it->second.ignored = true;
                    continue;
                }
                if ( do_import_restrictions_ ) { // static_if
                    // check if the node is involved in a restriction
                    if ( restrictions_->has_via_node( node ) ) {
                        restrictions_->add_node_edge( node, way_it->first );
                    }
                }
            }
            progressor( ++i, ways_.size() );
        }
    }

    ///
    /// Convert raw OSM ways to road sections. Sections are road parts between two intersections.
    template <typename Progressor>
    void write_sections( Writer& writer, Progressor& progressor )
    {
        progressor( 0, ways_.size() );
        size_t i = 0;
        writer.begin_sections();
        for ( auto way_it = ways_.begin(); way_it != ways_.end(); way_it++ ) {
            const Way& way = way_it->second;
            if ( way.ignored )
                continue;

            way_to_sections( way_it->first, way, writer );
            progressor( ++i, ways_.size() );
        }
        writer.end_sections();
    }

    template <typename Progressor>
    void write_nodes( Writer& writer, Progressor& progressor )
    {
        progressor( 0, points_.size() );
        size_t i = 0;
        writer.begin_nodes();
        for ( const auto& p : points_ ) {
            if ( p.second.uses() > 1 )
                writer.write_node( p.first, p.second.lat(), p.second.lon() );
            progressor( ++i, points_.size() );
        }
        writer.end_nodes();
    }

    void way_to_sections( uint64_t way_id, const Way& way, Writer& writer )
    {
        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = way.nodes[0];
        uint64_t node_from;
        std::vector<uint64_t> section_nodes;

        for ( size_t i = 1; i < way.nodes.size(); i++ ) {
            uint64_t node = way.nodes[i];
            const PointWithUses& pt = points_.at( node );
            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == way.nodes.size() - 1 || pt.uses() > 1 ) {
                section_splitter_( way_id, node_from, node, section_nodes, way.tags,
                                   [&](uint64_t lway_id, uint64_t lsection_id, uint64_t lnode_from, uint64_t lnode_to, const std::vector<Point>& lpts, const osm_pbf::Tags& ltags)
                                   {
                                       writer.write_section( lway_id, lsection_id, lnode_from, lnode_to, lpts, ltags );
                                       if ( do_import_restrictions_ ) { // static_if
                                           if ( restrictions_->has_via_node( lnode_from ) || restrictions_->has_via_node( lnode_to ) )
                                               restrictions_->add_way_section( lway_id, lsection_id, lnode_from, lnode_to );
                                       }
                                   });
                section_start = true;
            }
            old_node = node;
        }
    }

    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags & /*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    const PointCache& points() const { return points_; }

private:
    RestrictionReader* restrictions_;
    
    PointCache points_;
    WayCache ways_;

    SectionSplitter<PointCache> section_splitter_;
};


void single_pass_pbf_read( const std::string& filename, Writer& writer, bool do_write_nodes, bool do_import_restrictions )
{
    off_t ways_offset = 0, relations_offset = 0;
    osm_pbf::osm_pbf_offsets<StdOutProgressor>( filename, ways_offset, relations_offset );
    std::cout << "Ways offset: " << std::hex << ways_offset << std::endl;
    std::cout << "Relations offset: " << std::hex << relations_offset << std::endl;

    if ( do_import_restrictions ) {
        std::cout << "Relations ..." << std::endl;
        RestrictionReader r;
        osm_pbf::read_osm_pbf<RestrictionReader, StdOutProgressor>( filename, r, relations_offset );

        std::cout << "Nodes and ways ..." << std::endl;
        PbfReader<true> p( &r );
        osm_pbf::read_osm_pbf<PbfReader<true>, StdOutProgressor>( filename, p, 0, relations_offset );
        std::cout << "Marking nodes and ways ..."  << std::endl;
        {
            StdOutProgressor prog;
            p.mark_points_and_ways( prog );
        }
        std::cout << "Writing sections ..."  << std::endl;
        {
            StdOutProgressor prog;
            p.write_sections( writer, prog );
        }

        if ( do_write_nodes ) {
            std::cout << "Writing nodes..." << std::endl;
            StdOutProgressor prog;
            p.write_nodes( writer, prog );
        }
        std::cout << "Writing restrictions ..." << std::endl;
        {
            StdOutProgressor prog;
            r.write_restrictions( p.points(), writer, prog );
        }
    }
    else {
        std::cout << "Nodes and ways ..." << std::endl;
        PbfReader<false> p;
        osm_pbf::read_osm_pbf<PbfReader<false>, StdOutProgressor>( filename, p, 0, relations_offset );
        std::cout << "Marking nodes and ways ..."  << std::endl;
        {
            StdOutProgressor prog;
            p.mark_points_and_ways( prog );
        }
        std::cout << "Writing sections ..."  << std::endl;
        {
            StdOutProgressor prog;
            p.write_sections( writer, prog );
        }

        if ( do_write_nodes ) {
            std::cout << "Writing nodes..." << std::endl;
            StdOutProgressor prog;
            p.write_nodes( writer, prog );
        }
    }
}
