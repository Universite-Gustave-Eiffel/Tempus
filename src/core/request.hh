// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A Request is used to model user requests to the planning engine.
 */

#ifndef TEMPUS_REQUEST_HH
#define TEMPUS_REQUEST_HH

#include "common.hh"
#include "road_graph.hh"

#include <list>
#include <boost/any.hpp>

namespace Tempus
{
    class Request : public ConsistentClass
    {
    public:
	
	struct TimeConstraint
	{
	    enum TimeConstraintType
	    {
		NoConstraint = 0,
		ConstraintBefore,
		ConstraintAfter
	    };
	    int type; ///< TimeConstraintType

	    Date date;
	    Time time;
	};

	struct TransportSelection
	{
	    db_id_t type;

	    ///
	    /// private vehicule options
	    Road::Vertex parking_location;
	    bool must_reach_destination;

	    ///
	    /// public transport options : lis tof allowed networks
	    std::vector<db_id_t> allowed_networks;
	};

	///
	/// Class used to represent destinations of a request and constraints of the step
	struct Step
	{
	    Road::Vertex destination;

	    TimeConstraint departure_constraint;
	    TimeConstraint arrival_constraint;

	    ///
	    /// Allowed transport types.
	    std::list<TransportSelection> allowed_transport_types;
	};

	typedef std::vector<Step> StepList;
	///
	/// Steps involved in the request. It has to be made at a minimum of an origin and a destination. It may include intermediary points.
	StepList steps;

	///
	/// Vertex origin of the request
	Road::Vertex origin;

	///
	/// Shortcut to get the final destination (the last step)
	Road::Vertex get_destination() { return steps.back().destination; }
	
	///
	/// Criteria to optimize. The list is ordered by criterion priority
	std::vector<int> optimizing_criteria;

    protected:
	bool check_consistency_()
	{
	    EXPECT( steps.size() >= 1 );
	    EXPECT( optimizing_criteria.size() >= 1 );

	    // TODO : check consistency of step constraints ? 
	    return true;
	}
    };
}; // Tempus namespace

#endif
