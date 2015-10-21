#include "multimodal_graph_builder.hh"
#include "pgsql_importer.hh"

namespace Tempus
{

std::unique_ptr<RoutingData> MultimodalGraphBuilder::pg_import( const std::string& pg_options, ProgressionCallback& progression, const VariantMap& options )
{
    PQImporter importer( pg_options );

    bool consistency_check = true;
    auto consistency_it = options.find( "consistency_check" );
    if ( consistency_it != options.end() ) {
        consistency_check = consistency_it->second.as<bool>();
    }

    std::string schema_name = "tempus";
    auto schema_it = options.find( "schema_name" );
    if ( schema_it != options.end() ) {
        schema_name = schema_it->second.str();
    }

    std::unique_ptr<Multimodal::Graph> mm_graph( importer.import_graph( progression, consistency_check, schema_name ) );
    std::unique_ptr<RoutingData> rgraph( mm_graph.release() );
    return std::move(rgraph);
}

static bool register_me()
{
    std::unique_ptr<RoutingDataBuilder> builder( new MultimodalGraphBuilder() );
    RoutingDataBuilderRegistry::instance().addBuilder( std::move(builder) );
    return true;
}

bool init_ = register_me();

};
