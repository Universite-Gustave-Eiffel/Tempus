#ifndef TEMPUS_GRAPH_HH
#define TEMPUS_GRAPH_HH

#include <string>
#include <boost/graph/adjacency_list.hpp>

#define DEBUG

namespace Tempus {

    // TODO :
    // * doxygen doc
    // * beautify
    // * transform add_vertex( g ) to g.add_vertex()

    // Type used inside the DB to store IDs
    typedef long int db_id_t;

    struct Base
    {
	// persistant ID relative to the storage database
	// common to many classes
	db_id_t db_id;

	// Consistency checking
	// When on debug mode, declare a virtual function that could be overloaded
	// in derived classes
	// When the debug mode is disabled, no vtable is created
#ifdef DEBUG
	virtual bool check_consistency() { return true; }
#else
	bool check_consistency() { return true; }
#endif
    };
#define EXPECT( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Assertion " #expr " failed" << std::endl; return false; }}

    // Time is the number of seconds since 00:00
    typedef int Time;

    enum RoadType
    {
	RoadMotorway = 1,
	RoadPrimary,
	RoadSecondary,
	RoadStreet,
	RoadOther,
	RoadCycleWay,
	RoadPedestrial
    };

    struct TransportType
    {
	// must be a power of 2
	db_id_t id;
	db_id_t parent_id;

	std::string name;
	
	bool need_parking;
	bool need_station;
	bool need_return;

	TransportType() {}
	TransportType( db_id_t id, db_id_t parent_id, std::string name, bool need_parking, bool need_station, bool need_return ) :
	    id(id), parent_id(parent_id), name(name), need_parking(need_parking), need_station(need_station), need_return(need_return) {}
    };

    enum TransportTypeId
    {
	TransportCar = (1 << 0),
	TransportPedestrial = (1 << 1),
	TransportCycle = (1 << 2),
	TransportBus = (1 << 3),
	TransportTramway = (1 << 4),
	TransportMetro = (1 << 5),
	TransportTrain = (1 << 6),
	TransportSharedCycle = (1 << 7),
	TransportSharedCar = (1 << 8),
	TransportRoller = (1 << 9)
    };

    // forward declaration
    std::map<db_id_t, TransportType> init_transport_types_();
    const std::map<db_id_t, TransportType> transport_types = init_transport_types_();
    std::map<db_id_t, TransportType> init_transport_types_()
    {
	std::map<db_id_t, TransportType> t;
	t[ TransportCar ] = TransportType( TransportCar, 0, "Car", true, false, false ); 
	t[ TransportPedestrial ] = TransportType( TransportPedestrial, 0, "Pedestrial", false, false, false ); 
	t[ TransportCycle ] = TransportType( TransportCycle, 0, "Cycle", true, false, false ); 
	t[ TransportBus ] = TransportType( TransportBus, 0, "Bus", false, false, false ); 
	t[ TransportTramway ] = TransportType( TransportTramway, 0, "Tramway", false, false, false ); 
	t[ TransportMetro ] = TransportType( TransportMetro, 0, "Metro", false, false, false ); 
	t[ TransportTrain ] = TransportType( TransportTrain, 0, "Train", false, false, false ); 
	t[ TransportSharedCycle ] = TransportType( TransportSharedCycle, TransportCycle, "Shared cycle", true, true, false ); 
	t[ TransportSharedCar ] = TransportType( TransportSharedCar, TransportCar, "Shared car", true, true, false ); 
	t[ TransportRoller ] = TransportType( TransportRoller, TransportPedestrial | TransportCycle, "Roller", false, false, false ); 
	return t;
    }

    namespace Road
    {
	// storage types used to make a road graph
	typedef boost::vecS VertexListType;
	typedef boost::vecS EdgeListType;

	// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
	// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
	typedef typename boost::mpl::if_<typename boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
	// see adjacency_list.hpp
	typedef typename boost::detail::edge_desc_impl<boost::directed_tag, Vertex> Edge;

	/// used as Vertex
	/// refers to the 'road_node' DB's table
	struct Node : public Base
	{
	    // This is a shortcut to the vertex index in the corresponding graph, if any
	    // Needed to speedup access to a graph's vertex from a Node
	    // can be null
	    Vertex corresponding_vertex;

	    bool is_junction;
	    bool is_bifurcation;
	};
	
	/// used as Directed Edge
	/// refers to the 'road_section' DB's table
	struct Section : public Base
	{
	    // This is a shortcut to the edge index in the corresponding graph, if any
	    // Needed to speedup access to a graph's edge from a Section
	    // can be null
	    Edge corresponding_edge;

	    RoadType road_type;
	    int      transport_type; // bitfield of TransportTypeId
	    double   length;
	    double   car_speed_limit;
	    double   car_average_speed;
	    double   bus_average_speed;
	    std::string road_name;
	    std::string address_left_side;
	    std::string address_right_side;
	    int lane;
	    bool is_roundabout;
	    bool is_bridge;
	    bool is_tunnel;
	    bool is_ramp;
	    bool is_tollway;
	};
	
	/// The final road graph type
	typedef boost::adjacency_list<VertexListType, EdgeListType, boost::directedS, Node, Section > Graph;
    };

    /// refers to the 'poi' DB's table
    struct POI : public Base
    {
	enum PoiType
	{
	    TypeCarPark = 1,
	    TypeSharedCarPoint,
	    TypeCyclePark,
	    TypeSharedCyclePoint,
	    TypeUserPOI
	};
	int poi_type;

	std::string name;
	int parking_transport_type; // bitfield of TransportTypeId

	// link to a road section
	// must not be null
	Road::Section* road_section;
	double abscissa_road_section;

	bool check_consistency()
	{
	    EXPECT( road_section != 0 );
	    return true;
	}
    }; // Road namespace

    namespace PublicTransport
    {
	struct Network : public Base
	{
	    std::string name;
	};
	// list of available public transport networks
	std::vector< Network > networks;

	// storage types used to make a road graph
	typedef boost::vecS VertexListType;
	typedef boost::vecS EdgeListType;

	// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
	// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
	typedef typename boost::mpl::if_<typename boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
	// see adjacency_list.hpp
	typedef typename boost::detail::edge_desc_impl<boost::directed_tag, Vertex> Edge;

	/// used as a vertex in a PublicTransportGraph
	/// refers to the 'pt_stop' DB's table
	struct Stop : public Base
	{
	    // This is a shortcut to the vertex index in the corresponding graph, if any
	    // Needed to speedup access to a graph's vertex from a Node
	    // can be null
	    Vertex corresponding_vertex;

	    std::string name;
	    bool is_station;
	    
	    // link to a possible parent station, or null
	    Stop* parent_station;
	    
	    // link to a road section
	    // must not be null
	    Road::Section* road_section;
	    double abscissa_road_section;
	    
	    int zone_id;

	    bool check_consistency()
	    {
		EXPECT( road_section != 0 );
		return true;
	    }
	};

	/// used as an Edge in a PublicTransportGraph
	struct Section : public Base
	{
	    // This is a shortcut to the edge index in the corresponding graph, if any.
	    // Needed to speedup access to a graph's edge from a Section
	    // can be null
	    Edge corresponding_edge;
	    // must not be null
	    Network* network_id;
	};
	
	// definition of a public transport graph
	typedef boost::adjacency_list<VertexListType, EdgeListType, boost::directedS, Stop, Section> Graph;

	struct Trip : public Base
	{
	    std::string short_name;

	    /// partially refers to the 'pt_stop_time' table
	    struct StopTime
	    {
		Time arrival_time;
		Time departure_time;
		std::string stop_headsign;
		
		int pickup_type;
		int drop_off_type;
		double shape_dist_traveled;
	    };
	    
	    typedef std::vector<Stop*> StopSequence;
	    typedef std::list< std::vector< StopTime > > StopTimes;

	    // sequence of stops that describe the trip
	    StopSequence stop_sequence;
	    // list of all possible stop times
	    StopTimes stop_times;
	    
	    bool check_consistency()
	    {
		// check that the number of stops in a StopTime matches the number of stops of the trip
		for ( StopTimes::const_iterator it = stop_times.begin(); it != stop_times.end(); it++ )
		{
		    if ( it->size() != stop_sequence.size() )
			return false;
		}
		return true;
	    }
	};

	/// refers to the 'pt_route' DB's table
	struct Route : public Base
	{
	    // public transport network
	    Network* network_id;
	    
	    std::string short_name;
	    std::string long_name;
	    
	    enum RouteType
	    {
		TypeTram = 0,
		TypeSubway,
		TypeRail,
		TypeBus,
		TypeFerry,
		TypeCableCar,
		TypeSuspendedCar,
		TypeFunicular
	    };
	    int route_type;

	    std::vector<Trip> trips;

	    bool check_consistency()
	    {
		EXPECT( (route_type >= TypeTram) && (route_type <= TypeFunicular) );
		return true;
	    }
	};

	struct Transfer : public Base
	{
	    // link between two stops
	    // must not be null
	    Stop *from_stop, *to_stop;

	    enum TranferType
	    {
		NormalTransfer = 0,
		TimedTransfer,
		MinimalTimedTransfer,
		ImpossibleTransfer
	    };
	    int transfer_type;

	    // must be positive not null
	    // expressed in seconds
	    int min_transfer_time;

	    bool check_consistency()
	    {
		EXPECT( (transfer_type >= NormalTransfer) && (transfer_type <= ImpossibleTransfer) );
		EXPECT( min_transfer_time > 0 );
		return true;
	    }
	};
    }; // PublicTransport namespace

}; // Tempus namespace

#endif
