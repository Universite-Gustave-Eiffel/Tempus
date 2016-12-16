#include "routing_data_builder.hh"

namespace Tempus
{


void RoutingDataBuilderRegistry::addBuilder( std::unique_ptr<RoutingDataBuilder> a_builder )
{
    builders_[a_builder->name()] = std::move(a_builder);
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
    // On Windows, static and global variables are COPIED from the main module (EXE) to the other (DLL).
    // DLL have still access to the main EXE memory ...
    static RoutingDataBuilderRegistry* instance_ = 0;

    if ( 0 == instance_ ) {
#ifdef _WIN32
        // We test if we are in the main module (EXE) or not. If it is the case, a new Application is allocated.
        // It will also be returned by modules.
        RoutingDataBuilderRegistry* ( *main_get_instance )() = ( RoutingDataBuilderRegistry* (* )() )GetProcAddress( GetModuleHandle( NULL ), "get_routing_data_builder_registry__" );
        instance_ = ( main_get_instance == &get_routing_data_builder_registry_ )  ? new RoutingDataBuilderRegistry : main_get_instance();
#else
        instance_ = new RoutingDataBuilderRegistry();
#endif
    }

    return *instance_;
}

const char TEMPUS_DUMP_FILE_MAGIC[] = "TDBF";

void RoutingDataBuilder::write_header( std::ostream& ostr ) const
{
    ostr.write( TEMPUS_DUMP_FILE_MAGIC, 4 );
    char nname[256] = {0};
    strncpy( nname, name().c_str(), name().size() );
    ostr.write( nname, 256 );
    uint32_t v = version();
    ostr.write( reinterpret_cast<const char*>( &v ), sizeof( uint32_t ) );
}

void RoutingDataBuilder::read_header( std::istream& istr ) const
{
    char magic[5];
    istr.read( magic, 4 );
    magic[4] = 0;
    if ( std::string( magic ) != std::string( TEMPUS_DUMP_FILE_MAGIC ) ) {
        throw std::runtime_error( std::string( "Unrecognized header " ) + magic );
    }
    char nname[256];
    istr.read( nname, 256 );
    if ( std::string( nname ) != name() ) {
        throw std::runtime_error( "Bad routing data type " + std::string( nname ) + ", expected " + name() );
    }
    uint32_t v;
    istr.read( reinterpret_cast<char*>( &v ), sizeof( uint32_t ) );
    if ( v > version() ) {
        throw std::runtime_error( "Wrong version" );
    }
    std::cout << "Read header of type " << name() << std::endl;
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

RoutingData::TransportModes load_transport_modes( Db::Connection& connection )
{
    RoutingData::TransportModes modes;

    Db::Result res( connection.exec( "SELECT id, name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle FROM tempus.transport_mode" ) );

    for ( size_t i = 0; i < res.size(); i++ ) {
        db_id_t db_id=0; // avoid warning "may be used uninitialized"
        res[i][0] >> db_id;
        BOOST_ASSERT( db_id > 0 );
        // populate the global variable
        TransportMode t;
        t.set_db_id( db_id );
        t.set_name( res[i][1] );
        t.set_is_public_transport( res[i][2] );
        // gtfs_route_type ??
        if ( ! res[i][4].is_null() ) t.set_traffic_rules( res[i][4] );
        if ( ! res[i][5].is_null() ) t.set_speed_rule( static_cast<TransportModeSpeedRule>(res[i][5].as<int>()) );
        if ( ! res[i][6].is_null() ) t.set_toll_rules( res[i][6] );
        if ( ! res[i][7].is_null() ) t.set_engine_type( static_cast<TransportModeEngine>(res[i][7].as<int>()) );
        if ( ! res[i][8].is_null() ) t.set_need_parking( res[i][8] );
        if ( ! res[i][9].is_null() ) t.set_is_shared( res[i][9] );
        if ( ! res[i][10].is_null() ) t.set_must_be_returned( res[i][10] );

        modes[t.db_id()] = t;
    }

    return modes;
}

void load_metadata( RoutingData& rd, Db::Connection& connection )
{
    {
        // check that the metadata exists
        bool has_metadata = false;
        {
            Db::Result r( connection.exec( "select * from information_schema.tables where table_name='metadata' and table_schema='tempus'" ) );
            has_metadata = r.size() == 1;
        }
        if ( has_metadata ) {
            Db::Result res( connection.exec( "SELECT key, value FROM tempus.metadata" ) );
            for ( size_t i = 0; i < res.size(); i++ ) {
                std::string k, v;
                res[i][0] >> k;
                res[i][1] >> v;
                rd.set_metadata( k, v );
            }
        }
        else
        {
            // for versions of tempus databases before metadata have been introduced
            rd.set_metadata( "srid", "2154" );
        }
    }
}

}

