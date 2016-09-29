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

#ifndef OSM2TEMPUS_SQLITEWRITER_H
#define OSM2TEMPUS_SQLITEWRITER_H

#include "writer.h"
#include <sqlite3.h>


///
/// A Sqlite writer
class SqliteWriter : public Writer
{
public:
    SqliteWriter( const std::string& file_name, DataProfile* data_profile, bool keep_tags );
    
    virtual void write_section( uint64_t way_id, uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ ) override;

    ~SqliteWriter();

private:
    sqlite3* db;
    sqlite3_stmt* stmt;
};

#endif
