// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A Request is used to model user requests to the planning engine.
 */

#ifndef TEMPUS_REQUEST_HH
#define TEMPUS_REQUEST_HH

#include <list>

#include "common.hh"
#include "road_graph.hh"

namespace Tempus
{
    class Request : public ConsistentClass
    {
    public:
	///
	/// Steps involved in the request. It has to be made at a minimum of an origin and a destination. It may includes intermediary points.
	std::vector<Road::Node*> steps;

	///
	/// Allowed transport types. Because transport types are all powers of 2. It can be expressed by means of an integer.
	unsigned allowed_transport_types;

	///
	/// Criteria to optimize. The list is ordered by criterion priority
	std::vector<int> optimizing_criteria;

	bool check_consistency()
	{
	    EXPECT( steps.size() >= 2 );
	    EXPECT( optimizing_criteria.size() >= 1 );
	    return true;
	}
    };
}; // Tempus namespace

#endif
