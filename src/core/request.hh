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

	///
	/// Class used to represent destinations of a request and constraints of the step
	struct Step
	{
	    Road::Vertex destination;

	    TimeConstraint departure_constraint;
	    TimeConstraint arrival_constraint;
	};

	typedef std::vector<Step> StepList;
	///
	/// Steps involved in the request. It has to be made at a minimum of an origin and a destination. It may includes intermediary points.
	StepList steps;

	///
	/// Vertex origin of the request
	Road::Vertex origin;

	///
	/// Shortcut to get the final destination (the last step)
	Road::Vertex get_destination() { return steps.back().destination; }
	
	///
	/// Allowed transport types. Because transport types are all powers of 2. It can be expressed by means of an integer.
	unsigned allowed_transport_types;

	///
	/// When private transports are used, the request must specify where the private vehicules are parked.
	/// A std::map is used here where a Tempus::TransportType is associated to a Road::Vertex.
	/// The map must contains an entry for each selected transport types where a parking is needed (see Tempus::TransportType::need_parking)
	/// TODO: replace by a more generic "option" associated to each transport type ?? (based on a map<int, boost::any> ?)
	std::map<unsigned, Road::Vertex> parking_location;

	///
	/// Criteria to optimize. The list is ordered by criterion priority
	std::vector<int> optimizing_criteria;

	///
	/// Default constructor
	Request() : allowed_transport_types( 0xffff ) {}

    protected:
	bool check_consistency_()
	{
	    EXPECT( steps.size() >= 1 );
	    EXPECT( optimizing_criteria.size() >= 1 );
	    
	    std::map<db_id_t, TransportType>::const_iterator it;
	    // For each defined transport type of the global transport_types map ...
	    for ( it = transport_types.begin(); it != transport_types.end(); it++ )
	    {
		// ... if the request contains it ...
		if ( allowed_transport_types & it->first )
		{
		    std::cout << it->first << " included in " << allowed_transport_types << " " << it->second.name << std::endl;
		    // ... and if this type needs a parking ...
		    if ( it->second.need_parking )
		    {
			// ... we expect the parking location to be specified.
			EXPECT( parking_location.find( it->first ) != parking_location.end() );
		    }
		}
	    }

	    // TODO : check consistency of step constraints ? 
	    return true;
	}
    };
}; // Tempus namespace

#endif
