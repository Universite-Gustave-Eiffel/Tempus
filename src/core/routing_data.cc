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


boost::optional<TransportMode> RoutingData::transport_mode( db_id_t id ) const
{
    TransportModes::const_iterator fit = transport_modes_.find( id );
    if ( fit == transport_modes_.end() ) {
        return boost::optional<TransportMode>();
    }
    return fit->second;
}

boost::optional<TransportMode> RoutingData::transport_mode( const std::string& name ) const
{
    NameToId::const_iterator fit = transport_mode_from_name_.find( name );
    if ( fit == transport_mode_from_name_.end() ) {
        return boost::optional<TransportMode>();
    }
    return transport_modes_.find(fit->second)->second;
}

void RoutingData::set_transport_modes( const TransportModes& tm )
{
    transport_modes_ = tm;
    // cache name to id
    transport_mode_from_name_.clear();
    for ( TransportModes::const_iterator it = transport_modes_.begin(); it != transport_modes_.end(); ++it ) {
        transport_mode_from_name_[ it->second.name() ] = it->first;
    }
}

std::string RoutingData::metadata( const std::string& key ) const
{
    auto it = metadata_.find( key );
    if ( it == metadata_.end() ) {
        return "";
    }
    return it->second;
}

const std::map<std::string, std::string>& RoutingData::metadata() const
{
    return metadata_;
}

void RoutingData::set_metadata( const std::string& key, const std::string& value )
{
    metadata_[key] = value;
}

boost::optional<const PublicTransport::Network&> RoutingData::network( db_id_t id ) const
{
    NetworkMap::const_iterator it = network_map_.find(id);
    if ( it == network_map_.end() ) {
        return boost::optional<const PublicTransport::Network&>();
    }
    return it->second;
}

} // namespace Tempus
