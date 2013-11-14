/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *   
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_REQUEST_HH
#define TEMPUS_REQUEST_HH

#include "common.hh"
#include "road_graph.hh"

#include <list>
#include <boost/any.hpp>

namespace Tempus
{
    /**
       A Request is used to model user requests to the planning engine.
    */
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

	    DateTime date_time;
	};

	///
	/// Class used to represent destinations of a request and constraints of the step
	struct Step
	{
	    Road::Vertex destination;

	    TimeConstraint constraint;

	    ///
	    /// Whether the private vehicule must reach the destination
	    bool private_vehicule_at_destination;
	};

	typedef std::vector<Step> StepList;
	///
	/// Steps involved in the request. It has to be made at a minimum of an origin and a destination. It may include intermediary points.
	StepList steps;

	///
	/// Allowed transport types. It can be stored in an integer, since transport_type ID are powers of two.
	unsigned allowed_transport_types;
	
	///
	/// Private vehicule option: parking location
	Road::Vertex parking_location;

	///
	/// Public transport options: list of allowed networks
	std::vector<db_id_t> allowed_networks;
	
	///
	/// Timeing constraint on the departure
	TimeConstraint departure_constraint;
	///
	/// Vertex origin of the request
	Road::Vertex origin;

	///
	/// Shortcut to get the final destination (the last step)
	Road::Vertex destination() { return steps.back().destination; }
	
	///
	/// Criteria to optimize. The list is ordered by criterion priority.
	/// Refers to a CostId (see common.hh)
	std::vector<int> optimizing_criteria;

    protected:
	bool check_consistency_()
	{
	    REQUIRE( steps.size() >= 1 );
	    REQUIRE( optimizing_criteria.size() >= 1 );

	    //
	    // For each step, check correct timing causalities on constraints
	    TimeConstraint last = departure_constraint; (void)last;
	    for ( StepList::const_iterator it = steps.begin(); it != steps.end(); it++ )
	    {
		REQUIRE( it->constraint.date_time >= last.date_time );
	    }
	    return true;
	}
    };
} // Tempus namespace

#endif
