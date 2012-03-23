// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A Roadmap is an object used to model steps involved in a multimodal route.
   It is a base for result values of a request.
 */

#ifndef TEMPUS_ROADMAP_HH
#define TEMPUS_ROADMAP_HH

#include <map>

#include "common.hh"

namespace Tempus
{
    class Roadmap
    {
    public:
	///
	/// A Step that occurs on the road, either by a pedestrian or a private vehicle
	struct RoadStep
	{
	    ///
	    /// The road section where to start from
	    Road::Edge road_section;
	    ///
	    /// The road section where to go in the direction of
	    Road::Edge road_direction;
	    ///
	    /// Distance to walk/drive (in km). -1 if we have to go until the end of the section
	    ///
	    double distance_km;
	    /// The movement that is to be done at the end of the section
	    enum EndMovement
	    {
		TurnLeft,
		TurnRight,
		GoAhead,
		UTurn,
		FirstExit, ///< in a roundabout
		SecondExit,
		ThirdExit,
		ForthExit,
		FifthExit,
		SixthExit,
		YouAreArrived = 999
	    };
	    EndMovement end_movement;
	};

	///
	/// A Step made with a public transport
	struct PublicTransportStep
	{
	    PublicTransport::Vertex departure_stop;
	    PublicTransport::Vertex arrival_stop;
	    db_id_t trip_id; ///< used to indicate the direction
	};

	///
	/// A Step is a part of a route, where the transport type is constant
	/// This a generic class
	struct Step
	{
	    TransportTypeId transport_type;
	    Costs costs;

	    ///
	    /// It is either a step made of road steps or public transport steps.
	    /// TODO: if it is too memory-demanding, consider using derived classes (Step <= RoadStep and Step <= PublicTransportStep)
	    RoadStep road;
	    PublicTransportStep pt;
	};

	///
	/// A Roadmap is a list of Step augmented with some total costs.
	typedef std::vector<Step> StepList;
	StepList steps;
	Costs total_costs;
    };

    ///
    /// A Result is a list of Roadmap, ordered by relevance towards optimizing criteria
    typedef std::list<Roadmap> Result;

}; // Tempus namespace

#endif
