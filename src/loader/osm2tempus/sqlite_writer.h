#ifndef OSM2TEMPUS_SQLITEWRITER_H
#define OSM2TEMPUS_SQLITEWRITER_H

#include "writer.h"
#include <sqlite3.h>


///
/// A Sqlite writer
class SqliteWriter : public Writer
{
public:
    SqliteWriter( const std::string& file_name );
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ );

    ~SqliteWriter();

private:
    uint64_t section_id;
    sqlite3* db;
    sqlite3_stmt* stmt;
};

#endif
