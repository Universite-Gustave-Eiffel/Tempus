#ifndef OSM2TEMPUS_H
#define OSM2TEMPUS_H

#include <vector>
#include <unordered_map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "osmpbfreader.h"
#pragma GCC diagnostic pop

namespace osm_pbf = CanalTP;

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
        return (ulat.bits & 1) << 1 | (ulon.bits & 1);
    }
    void set_uses( int uses )
    {
        ulat.bits |= uses & 2;
        ulon.bits |= uses & 1;
    }
private:
    union {
        float v;
        uint32_t bits;
    } ulon;
    union {
        float v;
        uint32_t bits;
    } ulat;
};

struct Way
{
    std::vector<uint64_t> nodes;
    osm_pbf::Tags tags;
    bool ignored = false;
};


using PointCache = std::unordered_map<uint64_t, Point>;
using WayCache = std::unordered_map<uint64_t, Way>;

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
        std::cout << p / double(length) * 100.0 << "%";
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
