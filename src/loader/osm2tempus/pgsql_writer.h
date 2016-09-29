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

#include "writer.h"
#include "db.hh"

///
/// A binary COPY-based SQL writer
class SQLBinaryCopyWriter : public Writer
{
public:
    SQLBinaryCopyWriter( const std::string& db_params,
                         const std::string& schema,
                         const std::string& sections_table,
                         const std::string& nodes_table,
                         const std::string& restrictions_table,
                         bool create_table,
                         bool truncate, 
                         DataProfile* profile,
                         bool keep_tags );
    ~SQLBinaryCopyWriter();
    
    virtual void begin_nodes() override;
    virtual void write_node( uint64_t node_id, float lat, float lon );
    virtual void end_nodes() override;

    virtual void begin_sections() override;
    virtual void write_section( uint64_t way_id, uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) override;
    virtual void end_sections() override;

    virtual void begin_restrictions() override;
    virtual void write_restriction( uint64_t restriction_id, const std::vector<uint64_t>& sections ) override;
    virtual void end_restrictions() override;

private:
    Db::Connection db;
    std::string schema_, sections_table_, nodes_table_, restrictions_table_;
    bool create_table_;
    bool truncate_;
    uint64_t n_sections_;
};

