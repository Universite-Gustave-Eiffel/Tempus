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

#ifndef OSM2TEMPUS_WRITER_H
#define OSM2TEMPUS_WRITER_H

#include "osm2tempus.h"
#include "data_profile.h"

class Writer
{
public:
    Writer( DataProfile* profile = nullptr, bool keep_tags = false ) : data_profile_( profile ), keep_tags_( keep_tags ) {}
    
    virtual void begin_sections() {}
    virtual void write_section( uint64_t /*way_id*/, uint64_t /*section_id*/, uint64_t /*node_from*/, uint64_t /*node_to*/, const std::vector<Point>& /*points*/, const osm_pbf::Tags& /*tags*/ ) {}
    virtual void end_sections() {}

    virtual void begin_nodes() {}
    virtual void write_node( uint64_t /*node_id*/, float /*lat*/, float /*lon*/ ) {}
    virtual void end_nodes() {}

    virtual void begin_restrictions() {}
    virtual void write_restriction( uint64_t /*restriction_id*/, const std::vector<uint64_t>& /*section_ids*/ ) {}
    virtual void end_restrictions() {}

    virtual ~Writer() {}
protected:
    DataProfile* data_profile_;
    bool keep_tags_;
};

#endif
