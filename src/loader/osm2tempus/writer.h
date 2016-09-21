#ifndef OSM2TEMPUS_WRITER_H
#define OSM2TEMPUS_WRITER_H

#include "osm2tempus.h"
#include "data_profile.h"

class Writer
{
public:
    Writer( DataProfile* profile = nullptr, bool keep_tags = false ) : data_profile_( profile ), keep_tags_( keep_tags ) {}
    
    virtual void write_section( uint64_t /*node_from*/, uint64_t /*node_to*/, const std::vector<Point>& /*points*/, const osm_pbf::Tags& /*tags*/ )
    {
    }
    virtual void write_restriction( std::vector<uint64_t> /*section_ids*/ )
    {
    }
    virtual ~Writer() {};
protected:
    DataProfile* data_profile_;
    bool keep_tags_;
};

#endif
