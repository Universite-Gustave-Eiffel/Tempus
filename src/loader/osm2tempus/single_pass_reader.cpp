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

template <typename PointCacheType, bool do_import_restrictions_ = false>
struct PbfReader
{
    PbfReader( RestrictionReader* restrictions = 0, size_t n_nodes = 0, size_t n_ways = 0 ) :
        restrictions_( restrictions ),
        points_( n_nodes ),
        section_splitter_( points_ )
    {
        if ( n_ways )
            ways_.reserve( n_ways );
    }
    
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_.insert( osmid, typename PointCacheType::PointType(lon, lat) );
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
        n_ways_++;
    }

    template <typename Progressor>
    void mark_points_and_ways( Progressor& progressor )
    {
        progressor( 0, ways_.size() );
        size_t i = 0;
        for ( auto way_it = ways_.begin(); way_it != ways_.end(); way_it++ ) {
            // mark each nodes as being used
            size_t j = 0;
            for ( uint64_t node: way_it->second.nodes ) {
                auto pt = points_.find( node );
                if ( pt ) {
                    if ( j == 0 || j == way_it->second.nodes.size() - 1 ) {
                        // mark way's extermities
                        pt->set_uses( 2 );
                    }
                    else if ( pt->uses() < 2 ) {
                            pt->set_uses( pt->uses() + 1 );
                    }
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
                j++;
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
            progressor( ++i, ways_.size() );
            const Way& way = way_it->second;
            if ( way.ignored )
                continue;

            way_to_sections( way_it->first, way, writer );
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
            {
                writer.write_node( p.first, p.second.lat(), p.second.lon() );
            }
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

    const PointCacheType& points() const { return points_; }

    size_t n_ways() const { return n_ways_; }

private:
    RestrictionReader* restrictions_;
    
    PointCacheType points_;
    WayCache ways_;

    SectionSplitter<PointCacheType> section_splitter_;

    size_t n_ways_ = 0;
};


template<typename PointCacheType>
void single_pass_pbf_read_( const std::string& filename, Writer& writer, bool do_write_nodes, bool do_import_restrictions, size_t n_nodes, size_t n_ways )
{
    off_t ways_offset = 0, relations_offset = 0;
    osm_pbf::osm_pbf_offsets<StdOutProgressor>( filename, ways_offset, relations_offset );
    std::cout << "Ways offset: " << ways_offset << std::endl;
    std::cout << "Relations offset: " << relations_offset << std::endl;

    if ( n_nodes == 0 )
    {
        n_nodes = ways_offset / 5;
        std::cout << "# nodes estimation: " << std::dec << n_nodes << std::endl;
    }
    else {
        std::cout << "# nodes hint: " << std::dec << n_nodes << std::endl;
    }
    if ( n_ways == 0 )
    {
        n_ways = (relations_offset - ways_offset) / 220;
        std::cout << "# ways estimation: " << std::dec << n_ways << std::endl;
    }
    else {
        std::cout << "# ways hint: " << std::dec << n_ways << std::endl;
    }

    if ( do_import_restrictions ) {
        std::cout << "Relations ..." << std::endl;
        RestrictionReader r;
        osm_pbf::read_osm_pbf<RestrictionReader, StdOutProgressor>( filename, r, relations_offset );

        std::cout << "Nodes and ways ..." << std::endl;
        PbfReader<PointCacheType, true> p( &r, n_nodes, n_ways );
        osm_pbf::read_osm_pbf<PbfReader<PointCacheType, true>, StdOutProgressor>( filename, p, 0, relations_offset );
        std::cout << std::dec << p.points().size() << " nodes cached" << std::endl;
        std::cout << std::dec << p.n_ways() << " ways cached" << std::endl;
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
        PbfReader<PointCacheType, false> p( nullptr, n_nodes );
        osm_pbf::read_osm_pbf<PbfReader<PointCacheType, false>, StdOutProgressor>( filename, p, 0, relations_offset );
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

void single_pass_pbf_read( const std::string& filename, Writer& writer, bool do_write_nodes, bool do_import_restrictions, size_t n_nodes, size_t n_ways )
{
    // if we don't know anything about the number of nodes, use a dynamic point cache
    // else use a point cache with a preallocated size
    if ( n_nodes )
        single_pass_pbf_read_<SortedPointCache>( filename, writer, do_write_nodes, do_import_restrictions, n_nodes, n_ways );
    else
        single_pass_pbf_read_<PointCache>( filename, writer, do_write_nodes, do_import_restrictions, n_nodes, n_ways );
}
