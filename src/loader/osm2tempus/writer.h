#ifndef OSM2TEMPUS_WRITER_H
#define OSM2TEMPUS_WRITER_H

#include "osm2tempus.h"

class Writer
{
public:
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) = 0;
    virtual ~Writer() {};
};

#endif
