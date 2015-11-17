#include "routing_data_builder.hh"

namespace Tempus
{


void RoutingDataBuilderRegistry::addBuilder( std::unique_ptr<RoutingDataBuilder> builder )
{
    builders_[builder->name()] = std::move(builder);
}

const RoutingDataBuilder* RoutingDataBuilderRegistry::builder( const std::string& name )
{
    auto it = builders_.find( name );
    if ( it != builders_.end() ) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> RoutingDataBuilderRegistry::builder_list() const
{
    std::vector<std::string> names;
    for ( const auto& p : builders_ ) {
        names.push_back( p.first );
    }
    return names;
}

RoutingDataBuilderRegistry& RoutingDataBuilderRegistry::instance()
{
    static RoutingDataBuilderRegistry b;
    return b;
}

std::unique_ptr<RoutingData> RoutingDataBuilder::pg_import( const std::string& /*pg_options*/, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
    return std::unique_ptr<RoutingData>();
}
void RoutingDataBuilder::pg_export( const RoutingData* /*rd*/, const std::string& /*pg_options*/, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
}
std::unique_ptr<RoutingData> RoutingDataBuilder::file_import( const std::string& /*filename*/, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
    return std::unique_ptr<RoutingData>();
}
void RoutingDataBuilder::file_export( const RoutingData* /*rd*/, const std::string& /*filename*/, ProgressionCallback& /*progression*/, const VariantMap& /*options*/ ) const
{
}

}

