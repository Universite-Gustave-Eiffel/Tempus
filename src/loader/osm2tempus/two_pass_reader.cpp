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

#include "osm2tempus.h"
#include "writer.h"
#include "section_splitter.h"
#include "restrictions.h"

#include <unordered_set>

template <typename PointCacheType>
struct PbfReaderPass1
{
public:
    PbfReaderPass1( size_t n_nodes = 0 ) : points_( n_nodes )
    {
    }
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
            points_.inc_uses( node );
        }
        // mark way's extremities
        points_.inc_uses( nodes[0] );
        points_.inc_uses( nodes[nodes.size() - 1] );
    }
    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    PointCacheType& points() { return points_; }

    template <typename Progressor>
    void write_nodes( Writer& writer, Progressor& progressor )
    {
        progressor( 0, points_.size() );
        size_t i = 0;
        writer.begin_nodes();
        for ( auto it = points_.begin(); it != points_.end(); it++ ) {
            if ( points_.uses( it ) > 1 )
            {
                const auto& pt = points_.pt_from_it( it );
                writer.write_node( points_.id_from_it( it ), pt.lat(), pt.lon() );
            }
            progressor( ++i, points_.size() );
        }
        writer.end_nodes();
    }
private:
    PointCacheType points_;
};

template <typename PointCacheType, bool do_import_restrictions_ = true>
struct PbfReaderPass2
{
    PbfReaderPass2( PointCacheType& points, Writer& writer, RestrictionReader& restrictions ):
        restrictions_( restrictions ), points_( points ), writer_( writer ), section_splitter_( points_ )
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

            if ( do_import_restrictions_ ) {
                // check if the node is involved in a restriction
                if ( restrictions_.has_via_node( node ) ) {
                    restrictions_.add_node_edge( node, way_id );
                }                
            }
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
                                       if ( do_import_restrictions_ ) { // static_if
                                           if ( restrictions_.has_via_node( lnode_from ) || restrictions_.has_via_node( lnode_to ) )
                                               restrictions_.add_way_section( lway_id, lsection_id, lnode_from, lnode_to );
                                       }
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
    RestrictionReader& restrictions_;
    
    PointCacheType& points_;

    Writer& writer_;

    SectionSplitter<PointCacheType> section_splitter_;
};

template <typename PointCacheType, bool do_import_restrictions>
void two_pass_pbf_read_( const std::string& filename, Writer& writer, size_t n_nodes )
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

    RestrictionReader r;
    if ( do_import_restrictions ) {
        std::cout << "Relations ..." << std::endl;
        osm_pbf::read_osm_pbf<RestrictionReader, StdOutProgressor>( filename, r, relations_offset );
    }

    std::cout << "First pass ..." << std::endl;
    PbfReaderPass1<PointCacheType> p1( n_nodes );
    osm_pbf::read_osm_pbf<PbfReaderPass1<PointCacheType>, StdOutProgressor>( filename, p1, 0, relations_offset );
    std::cout << p1.points().size() << " nodes cached" << std::endl;
    std::cout << "Second pass ..." << std::endl;
    {
        PbfReaderPass2<PointCacheType, do_import_restrictions> p2( p1.points(), writer, r );
        osm_pbf::read_osm_pbf<PbfReaderPass2<PointCacheType, do_import_restrictions>, StdOutProgressor>( filename, p2, ways_offset, relations_offset );
    }
    std::cout << "Writing nodes ..." << std::endl;
    {
        StdOutProgressor prog;
        p1.write_nodes( writer, prog );
    }
    if ( do_import_restrictions ) {
        std::cout << "writing restrictions ..." << std::endl;
        StdOutProgressor prog;
        r.write_restrictions( p1.points(), writer, prog );
    }
}

void two_pass_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions, size_t n_nodes )
{
    if ( n_nodes ) {
        if ( do_import_restrictions )
            two_pass_pbf_read_<SortedPointCache, true>( filename, writer, n_nodes );
        else
            two_pass_pbf_read_<SortedPointCache, false>( filename, writer, n_nodes );
    }
    else {
        if ( do_import_restrictions )
            two_pass_pbf_read_<PointCache, true>( filename, writer, n_nodes );
        else
            two_pass_pbf_read_<PointCache, false>( filename, writer, n_nodes );
    }
}

void two_pass_vector_pbf_read( const std::string& filename, Writer& writer, bool do_import_restrictions )
{
    if ( do_import_restrictions )
        two_pass_pbf_read_<PointCacheVector, true>( filename, writer, 0 );
    else
        two_pass_pbf_read_<PointCacheVector, false>( filename, writer, 0 );
}








