#include "routing_data_builder.hh"

namespace Tempus
{

class MultimodalGraphBuilder : public RoutingDataBuilder
{
public:
    MultimodalGraphBuilder() : RoutingDataBuilder( "multimodal_graph" ) {}

    std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback&, const VariantMap& options = VariantMap() ) override;
};

}
