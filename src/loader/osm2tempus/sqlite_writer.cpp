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

#include "sqlite_writer.h"
#include <sstream>
#include "geom.h"

///
/// A Sqlite writer
SqliteWriter::SqliteWriter( const std::string& file_name, DataProfile* data_profile, bool keep_tags ) : Writer( data_profile, keep_tags )
{
    int r;
    r = sqlite3_open( file_name.c_str(), &db );
    if ( r != SQLITE_OK ) {
        throw std::runtime_error( "Problem opening " + file_name );
    }
    char *err_msg;
    // pragma cache_size ?
    // pragma page_size ?
    r = sqlite3_exec( db,
                      "pragma journal_mode = off;"
                      "pragma synchronous = off;"
                      "create table edges(id int, node_from int, node_to int, geom blob);"
                      "begin;", NULL, NULL, &err_msg );
    if ( r != SQLITE_OK ) {
        std::string msg = std::string("Problem on exec: ") + err_msg;
        throw std::runtime_error( msg );
    }
    r = sqlite3_prepare_v2( db, "insert into edges values (?,?,?,?)", -1, &stmt, NULL );
    if ( r != SQLITE_OK ) {
        throw std::runtime_error( "Problem during prepare" );
    }
    std::cout << "stmt = " << stmt << std::endl;
}
    
void SqliteWriter::write_section( uint64_t /*way_id*/, uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ )
{
    int r;
    r = sqlite3_bind_int64( stmt, 1, section_id );
    if ( r != SQLITE_OK ) {
        std::ostringstream err;
        err << "Problem during bind1 " << r;
        throw std::runtime_error( err.str() );
    }
    r = sqlite3_bind_int64( stmt, 2, node_from );
    if ( r != SQLITE_OK ) {
        throw std::runtime_error( "Problem during bind2" );
    }
    r = sqlite3_bind_int64( stmt, 3, node_to );
    if ( r != SQLITE_OK ) {
        throw std::runtime_error( "Problem during bind3" );
    }
    std::string ewkb = linestring_to_ewkb( points );
    r = sqlite3_bind_blob( stmt, 4, ewkb.data(), ewkb.length(), SQLITE_STATIC );
    if ( r != SQLITE_OK ) {
        throw std::runtime_error( "Problem during bind4" );
    }
    r = sqlite3_step( stmt );
    if ( r != SQLITE_DONE ) {
        throw std::runtime_error( "Problem during step" );
    }
    sqlite3_clear_bindings( stmt );
    sqlite3_reset( stmt );
}

SqliteWriter::~SqliteWriter()
{
    int r;
    char *err_msg;
    r = sqlite3_exec( db, "commit", NULL, NULL, &err_msg );
    if ( r != SQLITE_OK ) {
        std::string msg = std::string("Problem on exec: ") + err_msg;
        throw std::runtime_error( msg );
    }
    sqlite3_close( db );
}
