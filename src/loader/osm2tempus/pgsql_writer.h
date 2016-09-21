#include "writer.h"
#include "db.hh"

///
/// A basic text-based PostgreSQL writer
/// It prints statements to stdout
class SQLWriter : public Writer
{
public:
    SQLWriter();
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) override;
    
    ~SQLWriter();

private:
    uint64_t section_id;
};

///
/// A basic COPY-based SQL writer
class SQLCopyWriter : public Writer
{
public:
    SQLCopyWriter( const std::string& db_params );
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ );

    ~SQLCopyWriter();

private:
    uint64_t section_id;
    Db::Connection db;
};

///
/// A binary COPY-based SQL writer
class SQLBinaryCopyWriter : public Writer
{
public:
    SQLBinaryCopyWriter( const std::string& db_params, const std::string& schema, const std::string& table, bool create_table, DataProfile* profile, bool keep_tags );
    
    virtual void write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags );

    ~SQLBinaryCopyWriter();

private:
    uint64_t section_id;
    Db::Connection db;
    std::string schema_, table_;
    bool create_table_;
};

