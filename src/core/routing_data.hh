#ifndef TEMPUS_ROUTING_DATA_HH
#define TEMPUS_ROUTING_DATA_HH

#include "progression.hh"
#include "variant.hh"
#include "base.hh"
#include "property.hh"

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

    // transport modes
    // transport networks
    // metadata

    // 

    std::string name() const { return name_; }

private:
    std::string name_;
};

///
/// Load routing data given its name and options
/// The name is the name of the data builder
/// Routing data are stored globally
const RoutingData* load_routing_data( const std::string& data_name, ProgressionCallback& progression, const VariantMap& options = VariantMap() );

void dump_routing_data( const RoutingData* rd, const std::string& file_name, ProgressionCallback& progression );

}

#endif
