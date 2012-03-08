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
#include "road_graph.hh"

namespace Tempus
{
    namespace PublicTransport
    {
	struct Network : public Base
	{
	    std::string name;
	};

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
	    
	    ///
	    /// Fare zone ID of this stop
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
	
	///
	/// Refers to the 'pt_calendar' table
	struct Calendar : public Base
	{
	    bool monday;
	    bool tuesday;
	    bool wednesday;
	    bool thursday;
	    bool friday;
	    bool saturday;
	    bool sunday;

	    Date start_date, end_date;

	    ///
	    /// Refers to the 'pt_calendar_date' table.
	    /// It represents exceptions to the regular service
	    struct Exception : public Base
	    {
		Date calendar_date;
		
		enum ExceptionType
		{
		    ServiceAdded = 1,
		    ServiceRemoved
		};
		ExceptionType exception_type;
	    };

	    std::vector<Exception> service_exceptions;
	};
	
	///
	/// Trip, Route, StopTime and Frequencies classes
	///
	struct Trip : public Base
	{
	    std::string short_name;

	    ///
	    /// Refers to the 'pt_stop_time' table
	    struct StopTime : public Base
	    {
		///
		/// Link to the Stop. Must not be null.
		/// Represents the link part of the "stop_sequence" field
		Stop* stop;

		Time arrival_time;
		Time departure_time;
		std::string stop_headsign;
		
		int pickup_type;
		int drop_off_type;
		double shape_dist_traveled;
	    };

	    ///
	    /// Refers to the 'pt_frequency' table
	    struct Frequency : public Base
	    {
		Time start_time, end_time;
		int headways_secs;
	    };
	    
	    /// This is the definition of a list of stop times for a trip.
	    /// The list of stop times has to be ordered to represent the sequence of stops
	    /// (based on the "stop_sequence" field of the corresponding "stop_times" table
	    ///
	    /// This type can also be used as a roadmap for answers to planning requests
	    typedef std::list< std::vector< StopTime > > StopTimes;

	    typedef std::list<Frequency> Frequencies;

	    ///
	    /// List of all stop times. Can be a subset of those stored in the database.
	    StopTimes stop_times;
	    ///
	    /// List of frequencies for this trip
	    Frequencies frequencies;
	    
	    ///
	    /// Link to the dates when service is available.
	    /// Must not be null.
	    Calendar* service;

	    bool check_consistency()
	    {
		EXPECT( service != 0 );
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

	///
	/// Refers to the 'pt_fare_rule' table
	struct FareRule : public Base
	{
	    Route* route;

	    typedef std::vector<int> ZoneIdList;
	    ZoneIdList origins;
	    ZoneIdList destinations;
	    ZoneIdList contains;
	};
	
	struct FareAttribute : public Base
	{
	    char currency_type[4]; ///< ISO 4217 codes
	    double price;

	    enum TransferType
	    {
		NoTransferAllowed = 0,
		OneTransferAllowed,
		TwoTransfersAllowed,
		UnlimitedTransfers = -1
	    };
	    int transfers;

	    int transfers_duration; ///< in seconds

	    typedef std::vector<FareRule> FareRulesList;
	    FareRulesList fare_rules;

	    bool check_consistency()
	    {
		EXPECT( ((transfers >= NoTransferAllowed) && (transfers <= TwoTransfersAllowed)) || (transfers == -1) );
		return true;
	    }

	    FareAttribute()
	    {
		strncpy( currency_type, "EUR", 3 ); ///< default value
	    };
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
