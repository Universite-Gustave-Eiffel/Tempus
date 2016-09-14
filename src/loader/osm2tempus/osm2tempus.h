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
    Point( double mlon, double mlat ) : lon( mlon ), lat( mlat ) {}
    double lon, lat;
    int uses = 0;
};

struct Way
{
    std::vector<uint64_t> nodes;
    osm_pbf::Tags tags;
    bool ignored = false;
};


using PointCache = std::unordered_map<uint64_t, Point>;

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

#endif
