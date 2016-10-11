#include "plugin_factory.hh"
#include "isochrone_plugin.h"
#include "mm_lib/cost_calculator.hh"
#include "mm_lib/algorithms.hh"

#include <functional>

namespace Tempus
{

IsochronePlugin::IsochronePlugin( ProgressionCallback& progression, const VariantMap& options )
    : Plugin( "isochrone_plugin", options )
{
    // load graph
    const RoutingData* rd = load_routing_data( "multimodal_graph", progression, options );
    graph_ = dynamic_cast<const Multimodal::Graph*>( rd );
    if ( graph_ == nullptr ) {
        throw std::runtime_error( "Problem loading the multimodal graph" );
    }
}

std::unique_ptr<PluginRequest> IsochronePlugin::request( const VariantMap& options ) const
{
    return std::unique_ptr<PluginRequest>( new IsochronePluginRequest( this, options, graph_ ) );
}

IsochronePluginRequest::IsochronePluginRequest( const IsochronePlugin* plugin, const VariantMap& options, const Multimodal::Graph* graph )
    : PluginRequest( plugin, options ), graph_(graph)
{
}

// Data structure used to label vertices : (vertex, transport_mode)
struct VertexLabel
{
    Multimodal::Vertex vertex;
    db_id_t mode;

    bool operator<( const VertexLabel& other ) const
    {
        return vertex == other.vertex ? mode < other.mode : vertex < other.vertex;
    }
    bool operator==( const VertexLabel& other ) const
    {
        return vertex == other.vertex && mode == other.mode;
    }
};
} // namespace Tempus

namespace std
{
template <>
struct hash<Tempus::VertexLabel>
{
    size_t operator()( const Tempus::VertexLabel& d ) const
    {
        return hash<Tempus::Multimodal::Vertex>()( d.vertex ) ^ ( hash<Tempus::db_id_t>()( d.mode ) << 1 );
    }
};
}

namespace Tempus
{
//
// Data structure used inside the dijkstra-like algorithm
class MMVertexData
{
    DECLARE_RW_PROPERTY( potential, float );
    DECLARE_RW_PROPERTY( wait_time, float );
    DECLARE_RW_PROPERTY( shift_time, float );
    DECLARE_RW_PROPERTY( trip, db_id_t );
    DECLARE_RW_PROPERTY( predecessor, VertexLabel );
public:
    MMVertexData() : // default values
        potential_( std::numeric_limits<float>::max() ),
        wait_time_( 0.0 ),
        shift_time_( 0.0 ),
        trip_( 0 )
    {}
};

typedef std::unordered_map<VertexLabel, MMVertexData> MMVertexDataMap;

// IsochroneVisitor
struct IsochroneVisitor
{
public:
    // exception thrown when the isochrone limit is reached
    struct StopException {};
    
    IsochroneVisitor( const MMVertexDataMap& v_map, double limit, size_t& iterations ) : v_map_( v_map ), iterations_( iterations ), limit_( limit ) {}

    void examine_vertex( const VertexLabel& l, const Multimodal::Graph& )
    {
        auto it = v_map_.find( l );
        if ( it != v_map_.end() ) {
            if ( it->second.potential() >= limit_ )
                throw StopException();
        }
        iterations_++;
    }

    void finish_vertex( const VertexLabel&, const Multimodal::Graph& ) {}
    void discover_vertex( const VertexLabel&, const Multimodal::Graph& ) {}
    void examine_edge( const Multimodal::Edge&, const Multimodal::Graph& ) {}
    void edge_relaxed( const Multimodal::Edge&, unsigned int /*mode*/, const Multimodal::Graph& ) {}
    void edge_not_relaxed( const Multimodal::Edge&, unsigned int /*mode*/, const Multimodal::Graph& ) {}
private:
    const MMVertexDataMap& v_map_;
    size_t iterations_;
    double limit_;
};

std::unique_ptr<Result> IsochronePluginRequest::process( const Request& request )
{
    std::unique_ptr<Result> result( new Result );

    double isochrone_limit = get_float_option( "Isochrone/limit" );
    double min_transfer_time = get_float_option( "Time/min_transfer_time" );
    double walking_speed = get_float_option( "Time/walking_speed" );
    double cycling_speed = get_float_option( "Time/cycling_speed" );
    double car_parking_search_time = get_float_option( "Time/car_parking_search_time" );

    CostCalculator2 cost_calculator( request.steps()[1].constraint().date_time(),
                                     request.allowed_modes(),
                                     walking_speed,
                                     cycling_speed,
                                     min_transfer_time,
                                     car_parking_search_time,
                                     boost::none );
    
    MMVertexDataMap vertex_data_map;
    boost::associative_property_map< MMVertexDataMap > vertex_data_pmap( vertex_data_map );

    // we start on a road node
    Multimodal::Vertex origin = Multimodal::Vertex( *graph_, graph_->road_vertex_from_id(request.origin()).get(), Multimodal::Vertex::road_t() );

    // add each transport mode as a source
    std::vector<VertexLabel> sources;
    for ( auto mode: request.allowed_modes() ) {
        VertexLabel v = {origin, mode};
        sources.push_back(v);
        MMVertexData d;
        d.set_potential( 0.0 );
        d.set_predecessor( v );
        vertex_data_map[v] = d;
    }
    
    size_t iterations = 0;
    IsochroneVisitor vis( vertex_data_map, isochrone_limit, iterations );
    try {
        combined_ls_algorithm_no_init( *graph_, sources, vertex_data_pmap, cost_calculator, request.allowed_modes(), vis );
    }
    catch ( IsochroneVisitor::StopException& ) {
    }

    result->push_back( Isochrone() );
    Isochrone& isochrone = result->back().isochrone();
    for ( const auto& v : vertex_data_map ) {
        if ( v.second.potential() < isochrone_limit ) {
            Point3D pt = v.first.vertex.coordinates();
            isochrone.emplace_back( pt.x(), pt.y(), v.first.mode, v.second.potential() );
        }
    }
    
    return result;
}


} // namespace Tempus

DECLARE_TEMPUS_PLUGIN( "isochrone_plugin", Tempus::IsochronePlugin )





