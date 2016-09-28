#ifndef OSM2TEMPUS_RESTRICTIONS_H
#define OSM2TEMPUS_RESTRICTIONS_H

#include "geom.h"

#include <cmath>
#include <map>
#include <vector>
#include <unordered_map>
#include <set>

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

    bool has_via_node( uint64_t node ) const { return via_nodes_ways_.find( node ) != via_nodes_ways_.end(); }

    void add_node_edge( uint64_t node, uint64_t way )
    {
        via_nodes_ways_[node].push_back( way );
    }

    void add_way_section( uint64_t way_id, uint64_t section_id, uint64_t node1, uint64_t node2 )
    {
        way_sections_[way_id].emplace( section_id, node1, node2 );
    }

    template <typename Progressor, typename PointCacheType>
    void write_restrictions( const PointCacheType& points, Writer& writer, Progressor& progressor )
    {
        if ( restrictions_.size() == 0 )
            return;
        
        progressor( 0, restrictions_.size() );
        writer.begin_restrictions();
        size_t ri = 0;
        for ( const auto& tr : restrictions_ ) {
            progressor( ++ri, restrictions_.size() );
            // only process way - node - way relations
            if ( points.find( tr.from_way ) ||
                 !points.find( tr.via_node ) ||
                 points.find( tr.to_way ) )
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
            if ( section_from.id == 0 )
                continue;

            std::vector<Section> sections_to;
            for ( auto section: way_sections_[tr.to_way] ) {
                if ( section.node1 == tr.via_node || section.node2 == tr.via_node )
                    sections_to.push_back( section );
            }
            if ( sections_to.size() == 1 ) {
                section_to = sections_to[0];
                //std::cout << "1 " << section_from.id << " via " << tr.via_node << " to " << section_to.id << std::endl;
                writer.write_restriction( ++restriction_id, { section_from.id, section_to.id } );
            }
            else if ( sections_to.size() == 2 ) {
                // choose left or right
                float angle[2];
                int i = 0;
                for ( const auto& s : sections_to ) {
                    uint64_t n1 = s.node1;
                    uint64_t n2 = s.node2;
                    if ( tr.via_node == s.node2 )
                        std::swap( n1, n2 );
                    const auto& p1 = points.at( section_from.node1 );
                    const auto& p2 = points.at( tr.via_node );
                    const auto& p3 = points.at( n2 );
                    // compute the angle between the 3 points
                    // we could have used the orientation (determinant), but angle computation is more stable
                    angle[i] = angle_3_points( p1.lon(), p1.lat(), p2.lon(), p2.lat(), p3.lon(), p3.lat() );
                    i++;
                }
                if ( tr.type == TurnRestriction::NoLeftTurn || tr.type == TurnRestriction::OnlyLeftTurn ) {
                    // take the angle < 0
                    if ( angle[0] < 0 )
                        section_to = sections_to[0];
                    else
                        section_to = sections_to[1];
                }
                else if ( tr.type == TurnRestriction::NoRightTurn || tr.type == TurnRestriction::OnlyRightTurn ) {
                    // take the angle > 0
                    if ( angle[0] > 0 )
                        section_to = sections_to[0];
                    else
                        section_to = sections_to[1];
                }
                else if ( tr.type == TurnRestriction::NoStraightOn || tr.type == TurnRestriction::OnlyStraightOn ) {
                    // take the angle closer to 0
                    if ( abs(angle[0]) < abs(angle[1]) )
                        section_to = sections_to[0];
                    else
                        section_to = sections_to[1];
                }
                else {
                    std::cerr << "Ignoring restriction from " << tr.from_way << " to " << tr.to_way << " with type " << tr.type << std::endl;
                    continue;
                }

                // now distinguish between NoXXX and OnlyXXX restriction types
                if ( tr.type == TurnRestriction::NoLeftTurn ||
                     tr.type == TurnRestriction::NoRightTurn ||
                     tr.type == TurnRestriction::NoStraightOn ) {
                    // emit the restriction
                    //std::cout << "2 " << section_from.id << " via " << tr.via_node << " to " << section_to.id << std::endl;
                    writer.write_restriction( ++restriction_id, { section_from.id, section_to.id } );
                }
                else {
                    // OnlyXXX is like several NoXXX on the other edges
                    // we have to emit a restriction for every connected edges, except this one
                    for ( auto way : via_nodes_ways_.at( tr.via_node ) ) {
                        for ( auto section: way_sections_.at( way ) ) {
                            if ( section.node1 == tr.via_node || section.node2 == tr.via_node ) {
                                if ( section.id != section_to.id && section.id != section_from.id )
                                    //std::cout << "3 " << section_from.id << " via " << tr.via_node << " to " << section.id << std::endl;
                                    writer.write_restriction( ++restriction_id, { section_from.id, section.id } );
                            }
                        }
                    }
                }
                    
            }
        }
        writer.end_restrictions();
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
        Section() : node1(0), node2(0), id(0) {}
        Section( uint64_t sid, uint64_t n1, uint64_t n2 ) : node1(n1), node2(n2), id(sid) {}
        uint64_t node1, node2;
        uint64_t id;
        bool operator<( const Section& other ) const
        {
            return id < other.id;
        }
    };
    std::map<uint64_t, std::set<Section>> way_sections_;

    uint64_t restriction_id = 0;
};

#endif
