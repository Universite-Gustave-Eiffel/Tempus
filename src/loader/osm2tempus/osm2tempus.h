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

#ifndef OSM2TEMPUS_H
#define OSM2TEMPUS_H

#include <vector>
#include <unordered_map>
#include <iomanip>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "osmpbfreader.h"
#pragma GCC diagnostic pop

namespace osm_pbf = CanalTP;

///
/// Point class
/// sizeof(Point) = 8
struct Point
{
    Point() {}
    Point( float nlon, float nlat ) : lon_(nlon), lat_(nlat)
    {
    }
    float lat() const
    {
        return lat_;
    }
    float lon() const
    {
        return lon_;
    }
    void set_lat( float nlat )
    {
        lat_ = nlat;
    }
    void set_lon( float nlon )
    {
        lon_ = nlon;
    }
private:
    float lon_, lat_;
};

///
/// Point class
/// sizeof(Point) = 12
struct PointWithUses
{
    PointWithUses() : uses_( 0 ) {}
    PointWithUses( const Point& pt ) : lon_(pt.lon()), lat_(pt.lat()), uses_( 0 )  {}
    PointWithUses( float nlon, float nlat ) : lon_(nlon), lat_(nlat), uses_( 0 )
    {
    }
    float lat() const
    {
        return lat_;
    }
    float lon() const
    {
        return lon_;
    }
    void set_lat( float nlat )
    {
        lat_ = nlat;
    }
    void set_lon( float nlon )
    {
        lon_ = nlon;
    }
    int uses() const
    {
        return uses_;
    }
    void set_uses( int nuses )
    {
        uses_ = nuses;
    }

    // conversion to Point
    operator Point() const
    {
        return Point(lon_, lat_);
    }
private:
    float lon_, lat_;
    uint8_t uses_;
};

template <typename HashType>
class PointCacheT
{
public:
    using PointType = PointWithUses;

    PointCacheT( size_t n_elts = 0 )
    {
        if ( n_elts )
            points_.reserve( n_elts );
    }
    
    typename HashType::const_iterator begin() const
    {
        return points_.begin();
    }
    typename HashType::const_iterator end() const
    {
        return points_.end();
    }
    typename HashType::iterator begin()
    {
        return points_.begin();
    }
    typename HashType::iterator end()
    {
        return points_.end();
    }
    const PointType* find( uint64_t id ) const
    {
        auto it = points_.find( id );
        if ( it == points_.end() )
            return nullptr;
        return &it->second;
    }
    PointType* find( uint64_t id )
    {
        auto it = points_.find( id );
        if ( it == points_.end() )
            return nullptr;
        return &it->second;
    }

    size_t size() const
    {
        return points_.size();
    }

    uint64_t id_from_it( typename HashType::const_iterator it ) const
    {
        return it->first;
    }

    const PointType& pt_from_it( typename HashType::const_iterator it ) const
    {
        return it->second;
    }
    
    const PointType& at( uint64_t id ) const
    {
        return points_.at( id );
    }

    /// Insert a point with a given id
    void insert( uint64_t id, PointType&& point )
    {
        points_[id] = point;
        if ( id > max_id_ )
            max_id_ = id;
    }

    /// Insert a point with a given id
    void insert( uint64_t id, const PointType& point )
    {
        points_[id] = point;
        if ( id > max_id_ )
            max_id_ = id;
    }

    /// Insert a new point and return the new id
    uint64_t insert( const PointType& point )
    {
        points_[++max_id_] = point;
        return max_id_;
    }

    int uses( uint64_t id ) const
    {
        auto it = points_.find( id );
        return it->second.uses();
    }

    int uses( typename HashType::const_iterator it ) const
    {
        return it->second.uses();
    }

    void inc_uses( uint64_t id )
    {
        auto it = points_.find( id );
        if ( it == points_.end() )
            return;
        if ( it->second.uses() < 2 )
            it->second.set_uses( it->second.uses() + 1);
    }

private:
    uint64_t max_id_ = 0;
    HashType points_;
};

using STLPointCache = PointCacheT<std::unordered_map<uint64_t, PointWithUses>>;

class PointCacheVector
{
public:
    using PointType = Point;
    using CacheType = std::vector<PointType>;
    PointCacheVector( size_t = 0 )
    {
        size_t n_nodes = max_node_id_;
        std::cout << "preallocating " << n_nodes << " nodes" << std::endl;
        points_.reserve( n_nodes );
        uses_.resize( (n_nodes >> 2) + 1 );
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
    const PointType* find( uint64_t id ) const
    {
        return &points_[id];
    }
    PointType* find( uint64_t id )
    {
        return &points_[id];
    }

    size_t size() const
    {
        return points_.size();
    }

    uint64_t id_from_it( CacheType::const_iterator it ) const
    {
        return it - points_.begin();
    }

    const PointType& pt_from_it( CacheType::const_iterator it ) const
    {
        return *it;
    }

    const PointType& at( uint64_t id ) const
    {
        return points_[id];
    }
    PointType& at( uint64_t id )
    {
        return points_[id];
    }

    /// Insert a point with a given id
    void insert( uint64_t id, PointType&& point )
    {
        points_[id] = point;
    }

    /// Insert a point with a given id
    void insert( uint64_t id, const PointType& point )
    {
        points_[id] = point;
    }

    /// Insert a new point and return the new id
    uint64_t insert( const PointType& point )
    {
        uint64_t ret = last_node_id_;
        points_[last_node_id_--] = point;
        return ret;
    }

    int uses( uint64_t id ) const
    {
        const uint8_t p = (id % 4)*2;
        const uint8_t m = 3 << p;
        return (uses_[id >> 2] & m) >> p;
    }

    int uses( CacheType::const_iterator it ) const
    {
        return uses( it - points_.begin() );
    }

    void inc_uses( uint64_t id )
    {
        const uint8_t p = (id % 4)*2;
        const uint8_t m = 3 << p;
        uint8_t& v = uses_[id >> 2];
        uint8_t nuses = (v & m) >> p;
        if ( nuses < 2 )
            v |= (++nuses & 3) << p;
    }
private:
    CacheType points_;
    // node ids that are introduced to split multi edges
    // this should not overlap current OSM node ID (~ 2^32 in july 2016)
    const uint64_t max_node_id_ = 4700000000LL;
    uint64_t last_node_id_ = 4700000000LL;

    // vector of point uses
    // 2 bits by point
    std::vector<uint8_t> uses_;
};

namespace std
{
inline bool operator<( const std::pair<uint64_t, PointWithUses>& a, uint64_t v )
{
    return a.first < v;
}
}


///
/// A point cache where nodes are sorted
/// i.e. calls to insert always have a id greater than the previous call
class SortedPointCache
{
public:
    using PointType = PointWithUses;
    using CacheType = std::vector<std::pair<uint64_t, PointType>>;

    SortedPointCache( size_t n_elts = 0 )
    {
        if ( n_elts )
            points_.reserve( n_elts );
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
    const PointType* find( uint64_t id ) const
    {
        auto it = std::lower_bound( points_.begin(), points_.end(), id );
        if ( it->first == id )
            return &it->second;
        return nullptr;
    }
    PointType* find( uint64_t id )
    {
        auto it = std::lower_bound( points_.begin(), points_.end(), id );
        if ( it->first == id )
            return &it->second;
        return nullptr;
    }

    size_t size() const
    {
        return points_.size();
    }

    const PointType& at( uint64_t id ) const
    {
        return *find( id );
    }

    uint64_t id_from_it( CacheType::const_iterator it ) const
    {
        return it->first;;
    }

    const PointType& pt_from_it( CacheType::const_iterator it ) const
    {
        return it->second;
    }

    /// Insert a point with a given id
    void insert( uint64_t id, PointType&& point )
    {
        points_.emplace_back( id, point );
    }

    /// Insert a point with a given id
    void insert( uint64_t id, const PointType& point )
    {
        points_.emplace_back( id, point );
    }

    /// Insert a new point and return the new id
    uint64_t insert( const PointType& point )
    {
        points_.emplace_back( ++last_node_id_, point );
        return last_node_id_;
    }

    int uses( uint64_t id ) const
    {
        return find( id )->uses();
    }

    int uses( CacheType::const_iterator it ) const
    {
        return it->second.uses();
    }

    void inc_uses( uint64_t id )
    {
        auto it = find( id );
        if ( it == nullptr )
            return;
        if ( it->uses() < 2 )
            it->set_uses( it->uses() + 1);
    }

private:
    uint64_t last_node_id_ = 4700000000LL;
    CacheType points_;
};

// a pair of nodes
using node_pair = std::pair<uint64_t, uint64_t>;

namespace std {
  template <> struct hash<node_pair>
  {
    inline size_t operator()(const node_pair & np) const
    {
        // cf http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
        size_t h1 = hash<uint64_t>()( np.first );
        size_t h2 = hash<uint64_t>()( np.second );
        return h2 + 0x9e3779b9 + (h1<<6) + (h1>>2);
    }
  };
}

struct StdOutProgressor
{
    StdOutProgressor() : first_time( true ) {}
    static const int length = 40;
    void display_dots( int p )
    {
        std::cout << "[";
        for ( int i = 0; i < p; i++ ) {
            std::cout << ".";
        }
        for ( int i = p; i < length; i++ ) {
            std::cout << " ";
        }
        std::cout << "] ";
        std::cout << std::setw(4) << std::setfill(' ') << std::setprecision(1) << std::fixed << p / double(length) * 100.0 << "%";
        std::cout.flush();
    }
    void operator() ( size_t x, size_t total )
    {
        if ( total == 0 )
            return;
        if ( first_time ) {
            display_dots( 0 );
            first_time = false;
        }
        int p = (float(x) / float(total) * length);
        if ( p != n_dots ) {
            n_dots = p;
            std::cout << "\r";
            display_dots( p );
        }
    }
    ~StdOutProgressor()
    {
        std::cout << std::endl;
    }
private:
    bool first_time = true;
    int n_dots = 0;
};

#endif
