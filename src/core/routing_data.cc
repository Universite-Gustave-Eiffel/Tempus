#include "routing_data.hh"
#include "routing_data_builder.hh"

namespace Tempus
{

using RoutingDataRegistry = std::map<std::string, std::unique_ptr<RoutingData>>;

static RoutingDataRegistry& instance()
{
    static RoutingDataRegistry r;
    return r;
}

const RoutingData* load_routing_data( const std::string& name, ProgressionCallback& progression, const VariantMap& options )
{
    std::string key = name;
    for ( const auto& p : options ) {
        key += p.first + ":" + p.second.str();
    }

    auto it = instance().find( key );
    if ( it != instance().end() ) {
        return it->second.get();
    }

    const RoutingDataBuilder* builder = RoutingDataBuilderRegistry::instance().builder( name );
    if ( !builder ) {
        return nullptr;
    }

    // load from the db or from a file
    // with priority given to the dump file
    std::unique_ptr<RoutingData> routing_data;
    if ( options.find( "from_file" ) != options.end() ) {
        routing_data.reset( builder->file_import( options.find( "from_file" )->second.str(), progression, options ).release() );
        
    }
    else {
        std::string db_options;
        auto fit = options.find( "db/options" );
        if ( fit != options.end() ) {
            db_options = fit->second.str();
        }
        routing_data.reset( builder->pg_import( db_options, progression, options ).release() );
    }
    instance()[key] = std::move( routing_data );
    return instance()[key].get();
}

void dump_routing_data( const RoutingData* rd, const std::string& filename, ProgressionCallback& progression )
{
    const RoutingDataBuilder* builder = RoutingDataBuilderRegistry::instance().builder( rd->name() );
    builder->file_export( rd, filename, progression );
}

} // namespace Tempus
