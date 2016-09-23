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
                         DataProfile* profile,
                         bool keep_tags );
    
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
};

