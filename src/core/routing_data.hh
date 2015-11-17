#ifndef TEMPUS_ROUTING_DATA_HH
#define TEMPUS_ROUTING_DATA_HH

#include "progression.hh"
#include "variant.hh"
#include "base.hh"
#include "property.hh"
#include "transport_modes.hh"
#include "public_transport.hh"

#include <boost/optional.hpp>

namespace Tempus
{

///
/// Generic multimodal vertex, i.e. independent from a given graph implementation
/// Based on a unique id
class MMVertex
{
public:
    enum Type
    {
        Road,
        Transport,
        Poi
    };

    DECLARE_RO_PROPERTY( type, MMVertex::Type );

    DECLARE_RO_PROPERTY( id, db_id_t );

    boost::optional<db_id_t> network_id() const
    {
        if ( type_ == Transport ) {
            return network_id_;
        }
        return boost::optional<db_id_t>();
    }

    MMVertex( Type type, db_id_t id ) : type_(type), id_(id) {}
    MMVertex( db_id_t id, db_id_t network_id ) : type_(Transport), id_(id), network_id_(network_id) {}

private:
    db_id_t network_id_;
};

class MMEdge
{
public:
    MMEdge( const MMVertex& vertex1, const MMVertex& vertex2 ) : source_(vertex1), target_(vertex2) {}
    MMEdge( MMVertex&& vertex1, MMVertex&& vertex2 ) : source_(vertex1), target_(vertex2) {}

    DECLARE_RO_PROPERTY( source, MMVertex );
    DECLARE_RO_PROPERTY( target, MMVertex );
};

///
/// Generic class used for the storage of every data needed by plugins
/// It is usually composed of raw graph data
/// As well as additional data like index maps (to map vertex indexes to db id for instance)
class RoutingData
{
public:
    RoutingData( const std::string& name ) : name_(name) {}

    virtual ~RoutingData() {}

    ///
    /// Name of the data
    std::string name() const { return name_; }

    typedef std::map<db_id_t, TransportMode> TransportModes;
    ///
    /// Access to transport modes
    const TransportModes& transport_modes() const { return transport_modes_; }

    /// 
    /// Transport mode setter
    void set_transport_modes( const TransportModes& );

    ///
    /// access to a transportmode, given its id
    boost::optional<TransportMode> transport_mode( db_id_t id ) const;

    ///
    /// access to a transportmode, given its name
    boost::optional<TransportMode> transport_mode( const std::string& name ) const;


    ///
    /// access to a metadata
    std::string metadata( const std::string& key ) const;
    ///
    /// modification of a graph's metadata
    void set_metadata( const std::string& key, const std::string& value );
    ///
    /// access to all the metadata (read only)
    const std::map<std::string, std::string>& metadata() const;

    ///
    /// Public transport netwoks
    typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;

    ///
    /// Access to the network map
    NetworkMap network_map() const { return network_map_; }

    ///
    /// Network map setter
    void set_network_map( const NetworkMap& nm ) { network_map_ = nm; }

    /// Access to a particular network
    boost::optional<const PublicTransport::Network&> network( db_id_t ) const;
    
private:
    std::string name_;

protected:
    ///
    /// Graph metadata
    std::map<std::string, std::string> metadata_;

    TransportModes transport_modes_;

    NetworkMap network_map_;

private:
    typedef std::map<std::string, Tempus::db_id_t> NameToId;
    ///
    /// Associative array that maps a transport type name to a transport type id
    NameToId transport_mode_from_name_;
};

///
/// Load routing data given its name and options
/// The name is the name of the data builder
/// Routing data are stored globally
const RoutingData* load_routing_data( const std::string& data_name, ProgressionCallback& progression, const VariantMap& options = VariantMap() );

void dump_routing_data( const RoutingData* rd, const std::string& file_name, ProgressionCallback& progression );

}

#endif
