// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   This file contains declarations of classes used to model public transport graphs

   It generally maps to the database's schema: one class exists for each table.
   Tables with 1<->N arity are represented by STL containers (vectors or lists)
   External keys are represented by pointers to other classes.
   
   Road::Node and Road::Section classes are used to build a BGL public transport graph.
 */

#ifndef TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH
#define TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH

#include "common.hh"
#include <boost/graph/adjacency_list.hpp>

namespace Tempus
{
    namespace PublicTransport
    {
	struct Network : public Base
	{
	    std::string name;
	};
	///
	/// list of available public transport networks
	std::vector< Network > networks;

	///
	/// storage types used to make a road graph
	typedef boost::vecS VertexListType;
	typedef boost::vecS EdgeListType;

	///
	/// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
	/// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
	typedef typename boost::mpl::if_<typename boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
	/// see adjacency_list.hpp
	typedef typename boost::detail::edge_desc_impl<boost::directed_tag, Vertex> Edge;

	class Stop;
	class Section;
	///
	/// Definition of a public transport graph
	typedef boost::adjacency_list<VertexListType, EdgeListType, boost::directedS, Stop, Section> Graph;

	///
	/// Used as a vertex in a PublicTransportGraph.
	/// Refers to the 'pt_stop' DB's table
	struct Stop : public Base
	{
	    /// This is a shortcut to the vertex index in the corresponding graph, if any.
	    /// Needed to speedup access to a graph's vertex from a Node.
	    /// Can be null
	    Graph* graph;
	    Vertex vertex;

	    std::string name;
	    bool is_station;
	   
	    /// 
	    /// link to a possible parent station, or null
	    Stop* parent_station;
	    
	    /// link to a road section
	    /// must not be null
	    Road::Section* road_section;
	    double abscissa_road_section;
	    
	    int zone_id;

	    bool check_consistency()
	    {
		EXPECT( road_section != 0 );
		return true;
	    }
	};

	///
	/// used as an Edge in a PublicTransportGraph
	struct Section : public Base
	{
	    /// This is a shortcut to the edge index in the corresponding graph, if any.
	    /// Needed to speedup access to a graph's edge from a Section
	    /// Can be null
	    Graph* graph;
	    Edge edge;
	    /// must not be null
	    Network* network;
	};
	

	/// Trip, Route and StopTime classes
	///
	/// The mapping is here a bit different from the one used in the database.
	/// Basically a "Route" contains "Trips".
	/// Each "Trip" can be statically described by a sequence of "Stops".
	/// It can also be "instanciated" by attaching one or more StopTime to it
	/// (which is described by some timing and transfer properties on each stop)
	struct Trip : public Base
	{
	    std::string short_name;

	    ///
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

	    ///
	    /// sequence of stops that describe the trip
	    StopSequence stop_sequence;
	    /// list of all possible stop times
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

	///
	/// refers to the 'pt_route' DB's table
	struct Route : public Base
	{
	    ///
	    /// public transport network
	    Network* network;
	    
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
	    ///
	    /// Link between two stops.
	    /// Must not be null
	    Stop *from_stop, *to_stop;

	    enum TranferType
	    {
		NormalTransfer = 0,
		TimedTransfer,
		MinimalTimedTransfer,
		ImpossibleTransfer
	    };
	    int transfer_type;

	    /// Must be positive not null.
	    /// Expressed in seconds
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
