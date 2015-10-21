#ifndef TEMPUS_ROUTING_DATA_HH
#define TEMPUS_ROUTING_DATA_HH

#include "progression.hh"
#include "variant.hh"

namespace Tempus
{

///
/// Generic class used for the storage of every data needed by plugins
/// It is usually composed of raw graph data
/// As well as additional data like index maps (to map vertex indexes to db id for instance)
class RoutingData
{
public:
    virtual ~RoutingData() {}
};

using RoutingDataHandle = void*;

///
/// Load routing data given its name and options
/// The name is the name of the data builder
/// Routing data are stored globally
const RoutingData* load_routing_data_from_db( const std::string& data_name, const std::string& db_options, ProgressionCallback& progression, const VariantMap& options = VariantMap() );

}

#endif
