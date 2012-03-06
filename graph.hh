#ifndef TEMPUS_GRAPH_HH
#define TEMPUS_GRAPH_HH

#include <string>
#include <boost/graph/adjacency_list.hpp>

namespace Tempus {

    struct DBTable
    {
	// persistant ID relative to the storage database
	long int db_id;
    };

    namespace Road
    {
	/// RoadType
	typedef int RoadType;
	
	/// used as Vertex
	struct Node : public DBTable
	{
	    bool is_junction;
	    bool is_bifurcation;
	};
	
	/// used as Directed Edge
	struct Section : public DBTable
	{
	    RoadType road_type;
	    int      transport_type;
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
	
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Node, Section> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	typedef boost::graph_traits<Graph>::edge_descriptor Edge;
    };

    struct POI : public DBTable
    {
	int poi_type;
	std::string name;
	int parking_transport_type;

	// link to a road section
	boost::graph_traits<Road::Graph>::edge_descriptor road_section;
	double abscissa_road_section;
    }; // Road namespace

    namespace PublicTransport
    {
	/// used as a vertex in a PublicTransportGraph
	struct Stop : public DBTable
	{
	    std::string name;
	    bool is_station;
	    // TODO parent_station
	    
	    // link to a road section
	    boost::graph_traits<Road::Graph>::edge_descriptor road_section;	
	    double abscissa_road_section;
	    
	    int zone_id;
	};

	// TODO public transport newtorks
	
	struct Route : public DBTable
	{
	    // public transport network
	    int network_id;
	    
	    std::string short_name;
	    std::string long_name;
	    
	    enum RouteType
	    {
		Type_Tram = 0,
		Type_Subway,
		Type_Rail,
		Type_Bus,
		Type_Ferry,
		Type_Cable_Car,
		Type_Suspended_Car,
		Type_Funicular
	    };
	    int route_type;
	};

	/// used as a Node in a PublicTransportGraph
	struct Section : public DBTable
	{
	    int network_id;
	};
	
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Stop, Section> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	typedef boost::graph_traits<Graph>::edge_descriptor Edge;

	struct Trip : public DBTable
	{
	    // link to Route
	    Route* route;

	    std::string short_name;
	};

	struct Transfer : public DBTable
	{
	    // link between two graphs
	    Graph *from_graph, *to_graph;
	    boost::graph_traits<Graph>::vertex_descriptor from_stop, to_stop;
	    int transfer_type;
	};
    }; // PublicTransport namespace
}; // Tempus namespace

#endif
