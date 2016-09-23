#include <unordered_set>
#include <set>

#include "osm2tempus.h"
#include "writer.h"

struct TurnRestriction
{
    enum RestrictionType
    {
        NoLeftTurn,
        NoRightTurn,
        NoStraightOn,
        NoUTurn,
        OnlyRightTurn,
        OnlyLeftTurn,
        OnlyStraightOn,
        NoEntry,
        NoExit
    };
    RestrictionType type;
    uint64_t from_way;
    uint64_t via_node;
    uint64_t to_way;
};

///
/// Compute the (signed) angle (in degrees) between three points
float angle_3_points( float ax, float ay, float bx, float by, float cx, float cy )
{
    float abx = bx - ax; float aby = by - ay;
    float cbx = bx - cx; float cby = by - cy;

    float dot = (abx * cbx + aby * cby); // dot product
    float cross = (abx * cby - aby * cbx); // cross product

    return atan2(cross, dot) / M_PI * 180.;
}

struct RelationReader
{
    RelationReader() {}
    
    void node_callback( uint64_t /*osmid*/, double /*lon*/, double /*lat*/, const osm_pbf::Tags &/*tags*/ )
    {
    }

    void way_callback( uint64_t /*osmid*/, const osm_pbf::Tags& /*tags*/, const std::vector<uint64_t>& /*nodes*/ )
    {
    }
    
    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags & tags, const osm_pbf::References & refs )
    {
        auto r_it = tags.find( "restriction" );
        auto t_it = tags.find( "type" );
        if ( r_it != tags.end() &&
             t_it != tags.end() && t_it->second == "restriction" ) {
            uint64_t from = 0, via_n = 0, to = 0;
            for ( const osm_pbf::Reference& r : refs ) {
                if ( r.role == "from" )
                    from = r.member_id;
                else if ( r.role == "via" ) {
                    via_n = r.member_id;
                }
                else if ( r.role == "to" )
                    to = r.member_id;
            }

            if ( from && via_n && to ) {
                auto type_it = restriction_types.find( r_it->second );
                if ( type_it != restriction_types.end() ) {
                    restrictions_.push_back( TurnRestriction({type_it->second, from, via_n, to}) );
                    via_nodes_ways_.emplace( via_n, std::vector<uint64_t>() );
                }
            }
        }
    }

    bool has_node( uint64_t node ) const { return via_nodes_ways_.find( node ) != via_nodes_ways_.end(); }

    void add_node_edge( uint64_t node, uint64_t way )
    {
        std::cout << "add edge " << way << " to node " << node << std::endl;
        via_nodes_ways_[node].push_back( way );
    }

    void add_way_section( uint64_t way_id, uint64_t section_id, uint64_t node1, uint64_t node2 )
    {
        way_sections_[way_id].emplace( section_id, node1, node2 );
    }

    void resolve_restrictions( const PointCache& points )
    {
        for ( const auto& tr : restrictions_ ) {
            // only process way - node - way relations
            if ( points.find( tr.from_way ) != points.end() ||
                 points.find( tr.via_node ) == points.end() ||
                 points.find( tr.to_way ) != points.end() )
                continue;
            // get the first edge until on the "from" way
            Section section_from;
            Section section_to;
            for ( auto section: way_sections_[tr.from_way] ) {
                if ( section.node2 == tr.via_node ) {
                    section_from = section;
                    break;
                }
                if ( section.node1 == tr.via_node ) {
                    section_from.id = section.id;
                    section_from.node1 = section.node2;
                    section_from.node2 = section.node1;
                    break;
                }
            }
            if ( way_sections_[tr.to_way].size() == 1 ) {
                section_to = *way_sections_[tr.to_way].begin();
                //std::cout << section_from.node1 << " - " << section_from.node2 << " via " << tr.via_node << " to " << section_to.node1 << " - " << section_to.node2 << std::endl;
                std::cout << "1 " << section_from.id << " via " << tr.via_node << " to " << section_to.id << std::endl;
            }
            else if ( way_sections_[tr.to_way].size() == 2 ) {
                // choose left or right
                float angle[2];
                int i = 0;
                for ( const auto& s : way_sections_[tr.to_way] ) {
                    uint64_t n1 = s.node1;
                    uint64_t n2 = s.node2;
                    std::cout << section_from.node1 << " - " << n1 << " - " << n2 << " - " << std::endl;
                    if ( tr.via_node == s.node2 )
                        std::swap( n1, n2 );
                    const Point& p1 = points.find( section_from.node1 )->second;
                    const Point& p2 = points.find( tr.via_node )->second;
                    const Point& p3 = points.find( n2 )->second;
                    angle[i] = angle_3_points( p1.lon(), p1.lat(), p2.lon(), p2.lat(), p3.lon(), p3.lat() );
                    std::cout << "angle " << i << " = " << angle[i] << std::endl;
                    i++;
                }
            }
            else {
                std::cout << "ignoring restriction with " << way_sections_[tr.to_way].size() << " 'to' edges" << std::endl;
            }
        }
    }
    
private:
    const std::map<std::string, TurnRestriction::RestrictionType> restriction_types = {
        {"no_left_turn", TurnRestriction::NoLeftTurn},
        {"no_right_turn", TurnRestriction::NoRightTurn},
        {"no_straight_on", TurnRestriction::NoStraightOn},
        {"only_left_turn", TurnRestriction::OnlyLeftTurn},
        {"only_right_turn", TurnRestriction::OnlyRightTurn},
        {"only_straight_on", TurnRestriction::OnlyStraightOn},
        {"no_entry", TurnRestriction::NoEntry},
        {"no_exit", TurnRestriction::NoExit} };

    // maps a via node id to a list of ways that pass through it
    std::unordered_map<uint64_t, std::vector<uint64_t>> via_nodes_ways_;
    std::vector<TurnRestriction> restrictions_;
    struct Section
    {
        Section() {}
        Section( uint64_t sid, uint64_t n1, uint64_t n2 ) : node1(n1), node2(n2), id(sid) {}
        uint64_t node1, node2;
        uint64_t id;
        bool operator<( const Section& other ) const
        {
            return id < other.id;
        }
    };
    std::map<uint64_t, std::set<Section>> way_sections_;
};

struct PbfReader
{
    PbfReader( RelationReader& restrictions ) : restrictions_( restrictions ) {}
    
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_[osmid] = Point(lon, lat);
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
                auto it = points_.find( node );
                if ( it != points_.end() ) {
                    int uses = it->second.uses();
                    if ( uses < 2 )
                        it->second.set_uses( uses + 1 );
                }
                else {
                    // unknown point
                    way_it->second.ignored = true;
                }
                // check if the node is involved in a restriction
                if ( restrictions_.has_node( node ) ) {
                    restrictions_.add_node_edge( node, way_it->first );
                }
            }
        }
    }

    ///
    /// Convert raw OSM ways to road sections. Sections are road parts between two intersections.
    void write_sections( Writer& writer )
    {
        writer.begin_sections();
        for ( auto way_it = ways.begin(); way_it != ways.end(); way_it++ ) {
            const Way& way = way_it->second;
            if ( way.ignored )
                continue;

            way_to_sections( way_it->first, way, writer );
        }
        writer.end_sections();
    }

    void write_nodes( Writer& writer )
    {
        writer.begin_nodes();
        for ( const auto& p : points_ ) {
            if ( p.second.uses() > 1 )
                writer.write_node( p.first, p.second.lat(), p.second.lon() );
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
        //Point old_pt = points_.find( old_node )->second;
        for ( size_t i = 1; i < way.nodes.size(); i++ ) {
            uint64_t node = way.nodes[i];
            const Point& pt = points_.find( node )->second;
            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == way.nodes.size() - 1 || pt.uses() > 1 ) {
                split_into_sections( way_id, node_from, node, section_nodes, way.tags, writer );
                //writer.write_section( node_from, node, section_pts, way.tags );
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
    RelationReader& restrictions_;
    
    PointCache points_;
    WayCache ways;

    // structure used to detect multi edges
    std::unordered_set<node_pair> way_node_pairs;
    //std::unordered_map<node_pair, uint64_t> way_node_pairs;

    // node ids that are introduced to split multi edges
    // we count them backward from 2^64 - 1
    // this should not overlap current OSM node ID (~ 2^32 in july 2016)
    uint64_t last_artificial_node_id = 0xFFFFFFFFFFFFFFFFLL;

    uint64_t section_id_ = 0;

        ///
    /// Check if a section with the same (from, to) pair exists
    /// In which case, it is split into two sections
    /// in order to avoid multigraphs
    void split_into_sections( uint64_t way_id, uint64_t node_from, uint64_t node_to, const std::vector<uint64_t>& nodes, const osm_pbf::Tags& tags, Writer& writer )
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
                    before_pts.push_back( points_.find( nodes[i] )->second );
                }
                for ( size_t i = center; i < nodes.size(); i++ ) {
                    after_pts.push_back( points_.find( nodes[i] )->second );
                }
            }
            else {
                const Point& p1 = points_.find( nodes[0] )->second;
                const Point& p2 = points_.find( nodes[1] )->second;
                Point center_point( ( p1.lon() + p2.lon() ) / 2.0, ( p1.lat() + p2.lat() ) / 2.0 );
                
                before_pts.push_back( points_.find( nodes[0] )->second );
                before_pts.push_back( center_point );
                after_pts.push_back( center_point );
                after_pts.push_back( points_.find( nodes[1] )->second );

                // add a new point
                center_node = last_artificial_node_id;
                points_[last_artificial_node_id--] = center_point;
            }
            writer.write_section( ++section_id_, node_from, center_node, before_pts, tags );
            if ( restrictions_.has_node( node_from ) || restrictions_.has_node( center_node ) )
                restrictions_.add_way_section( way_id, section_id_, node_from, center_node );
            writer.write_section( ++section_id_, center_node, node_to, after_pts, tags );
            if ( restrictions_.has_node( center_node ) || restrictions_.has_node( node_to ) )
                restrictions_.add_way_section( way_id, section_id_, center_node, node_to );
        }
        else {
            way_node_pairs.insert( p );
            std::vector<Point> section_pts;
            for ( uint64_t node: nodes ) {
                section_pts.push_back( points_.find( node )->second );
            }
            writer.write_section( ++section_id_, node_from, node_to, section_pts, tags );
            if ( restrictions_.has_node( node_from ) || restrictions_.has_node( node_to ) )
                restrictions_.add_way_section( way_id, section_id_, node_from, node_to );
        }
    }
};


void single_pass_pbf_read( const std::string& filename, Writer& writer, bool do_write_nodes )
{
    off_t ways_offset = 0, relations_offset = 0;
    osm_pbf::osm_pbf_offsets<StdOutProgressor>( filename, ways_offset, relations_offset );
    std::cout << "Ways offset: " << ways_offset << std::endl;
    std::cout << "Relations offset: " << relations_offset << std::endl;

    std::cout << "Relations ..." << std::endl;
    RelationReader r;
    osm_pbf::read_osm_pbf<RelationReader, StdOutProgressor>( filename, r, relations_offset );

    std::cout << "Nodes and ways ..." << std::endl;
    PbfReader p( r );
    osm_pbf::read_osm_pbf<PbfReader, StdOutProgressor>( filename, p, 0, relations_offset );
    p.mark_points_and_ways();
    p.write_sections( writer );

    if ( do_write_nodes ) {
        std::cout << "Writing nodes..." << std::endl;
        p.write_nodes( writer );
    }

    r.resolve_restrictions( p.points() );
}
