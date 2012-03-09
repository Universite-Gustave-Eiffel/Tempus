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
	std::list<Road::Section*> steps;

	///
	/// Allowed transport types. Because transport types are all powers of 2. It can be expressed by means of an integer.
	unsigned allowed_transport_types;

	///
	/// Criteria to optimize. The list is ordered by criterion priority
	std::list<CostId> optimizing_criteria;

	bool check_consistency()
	{
	    EXPECT( steps.size() >= 2 );
	    return true;
	}
    };
}; // Tempus namespace

#endif
