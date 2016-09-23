#include "writer.h"
#include "db.hh"

///
/// A binary COPY-based SQL writer
class SQLBinaryCopyWriter : public Writer
{
public:
    SQLBinaryCopyWriter( const std::string& db_params, const std::string& schema, const std::string& table, const std::string& nodes_table, bool create_table, DataProfile* profile, bool keep_tags );
    
    virtual void begin_nodes() override;
    virtual void write_node( uint64_t node_id, float lat, float lon );
    virtual void end_nodes() override;

    virtual void begin_sections() override;
    virtual void write_section( uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) override;
    virtual void end_sections() override;

private:
    Db::Connection db;
    std::string schema_, table_, nodes_table_;
    bool create_table_;
};

