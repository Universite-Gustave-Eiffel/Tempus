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

const RoutingData* load_routing_data_from_db( const std::string& name, const std::string& db_options, ProgressionCallback& progression, const VariantMap& options )
{
    std::string key = name;
    for ( const auto& p : options ) {
        key += p.first + ":" + p.second.str();
    }

    auto it = instance().find( key );
    if ( it != instance().end() ) {
        return it->second.get();
    }

    RoutingDataBuilder* builder = RoutingDataBuilderRegistry::instance().builder( name );
    if ( !builder ) {
        return nullptr;
    }
    std::unique_ptr<RoutingData> d = builder->pg_import( db_options, progression, options );
    instance()[key] = std::move( d );
    return instance()[key].get();
}

} // namespace Tempus
