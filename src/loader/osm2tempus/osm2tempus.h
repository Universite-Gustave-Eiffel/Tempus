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

#ifdef OSM_POINT_APPROXIMATION
///
/// Point class where 1 bit of each coordinate is taken to store a counter
/// sizeof(Point) = 8
struct Point
{
    Point() {}
    Point( float mlon, float mlat )
    {
        set_lon( mlon );
        set_lat( mlat );
    }
    float lat() const
    {
        return ulat.v;
    }
    float lon() const
    {
        return ulon.v;
    }
    void set_lat( float lat )
    {
        ulat.v = lat;
        ulat.bits &= 0xFFFFFFFE;
    }
    void set_lon( float lon )
    {
        ulon.v = lon;
        ulon.bits &= 0xFFFFFFFE;
    }
    int uses() const
    {
        return ((ulat.bits & 1) << 1) | (ulon.bits & 1);
    }
    void set_uses( int uses )
    {
        ulat.bits = (ulat.bits & 0xFFFFFFFE) | ((uses & 2) >> 1);
        ulon.bits = (ulon.bits & 0xFFFFFFFE) | (uses & 1);
    }
private:
    union {
        float v;
        uint32_t bits = 0;
    } ulon;
    union {
        float v;
        uint32_t bits = 0;
    } ulat;
};
#else
///
/// Point class
/// sizeof(Point) = 12
struct Point
{
    Point() {}
    Point( float lon, float lat ) : lon_(lon), lat_(lat)
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
    void set_lat( float lat )
    {
        lat_ = lat;
    }
    void set_lon( float lon )
    {
        lon_ = lon;
    }
    int uses() const
    {
        return uses_;
    }
    void set_uses( int uses )
    {
        uses_ = uses;
    }
private:
    float lon_, lat_;
    uint8_t uses_;
};
#endif

struct Way
{
    std::vector<uint64_t> nodes;
    osm_pbf::Tags tags;
    bool ignored = false;
};


//using PointCache = std::unordered_map<uint64_t, Point>;
using WayCache = std::unordered_map<uint64_t, Way>;

class PointCache
{
public:
    using CacheType = std::unordered_map<uint64_t, Point>;
    
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
        return points_.find( id );
    }
    CacheType::iterator find( uint64_t id )
    {
        return points_.find( id );
    }

    size_t size() const
    {
        return points_.size();
    }

    const Point& at( uint64_t id ) const
    {
        return points_.at( id );
    }

    /// Insert a point with a given id
    void insert( uint64_t id, Point&& point )
    {
        points_[id] = point;
        if ( id > max_id_ )
            max_id_ = id;
    }

    /// Insert a point with a given id
    void insert( uint64_t id, const Point& point )
    {
        points_[id] = point;
        if ( id > max_id_ )
            max_id_ = id;
    }

    /// Insert a new point and return the new id
    uint64_t insert( const Point& point )
    {
        uint64_t ret = max_id_;
        points_[max_id_++] = point;
        return ret;
    }
private:
    uint64_t max_id_ = 0;
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
