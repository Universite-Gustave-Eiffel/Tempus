#include "routing_data_builder.hh"
#include "road_graph.hh"
#include "db.hh"

namespace Tempus
{

class MultimodalGraphBuilder : public RoutingDataBuilder
{
public:
    MultimodalGraphBuilder() : RoutingDataBuilder( "multimodal_graph" ) {}

    virtual std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback&, const VariantMap& options = VariantMap() ) const override;

    virtual std::unique_ptr<RoutingData> file_import( const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;
    virtual void file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const override;
};

Road::Restrictions import_turn_restrictions( Db::Connection& connection, const Road::Graph& graph, const std::string& schema_name = "tempus" );

}
