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
