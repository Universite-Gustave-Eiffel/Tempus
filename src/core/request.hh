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

#include <boost/optional.hpp>
#include "common.hh"
#include "road_graph.hh"

#include <list>

namespace Tempus {
/**
   A Request is used to model user requests to the planning engine.

   It is composed of at least an origin and a destination with optional time constraints.
   Intermediary steps can be added.
*/
class Request
{
public:

    struct TimeConstraint {
        enum TimeConstraintType {
            NoConstraint = 0,
            /// Arrive before
            ConstraintBefore,
            /// Depart after
            ConstraintAfter,
            LastValue = ConstraintAfter
        };
        DECLARE_RW_PROPERTY( type, TimeConstraintType );
        /// type setter from an int
        void type( int );
        DECLARE_RW_PROPERTY( date_time, DateTime );
    };

    ///
    /// Class used to represent destinations of a request and constraints of the step
    struct Step {
        DECLARE_RW_PROPERTY( location, Road::Vertex );
        ///
        /// Time constraint.
        /// @warning Should be ignored for the first step (origin)
        DECLARE_RW_PROPERTY( constraint, TimeConstraint );
        ///
        /// Whether the private vehicule must reach the destination
        DECLARE_RW_PROPERTY( private_vehicule_at_destination, bool );
    };
    typedef std::vector<Step> StepList;

    /// Construct a request with default values
    Request();

    ///
    /// Construct a request with an origin and a destination
    /// @param origin The origin
    /// @param destination The destination. The timing causality must be respected
    Request( const Step& origin, const Step& destination );

    ///
    /// Steps involved in the request. It is made of an origin, optional intermediary steps and a destination
    DECLARE_RO_PROPERTY( steps, StepList );

    ///
    /// Adds an intermediary step, just before the destination
    void add_intermediary_step( const Step& step );

    ///
    /// Get access to the origin vertex
    Road::Vertex origin() const;

    /// Get write access to the origin vertex
    void set_origin( const Road::Vertex& );
    /// Get write access to the origin
    void set_origin( const Step& );

    ///
    /// Get access to the destination vertex
    Road::Vertex destination() const;

    /// Get write access to the destination vertex
    void set_destination( const Road::Vertex& );
    /// Get write access to the destination
    void set_destination( const Step& );

    ///
    /// Allowed transport modes (their ID)
    DECLARE_RO_PROPERTY( allowed_modes, std::vector<db_id_t> );

    /// Adds an allowed mode
    void add_allowed_mode( db_id_t );

    ///
    /// Private vehicule option: parking location
    DECLARE_RW_PROPERTY( parking_location, boost::optional<Road::Vertex> );

    ///
    /// Criteria to optimize. The list is ordered by criterion priority.
    /// Refers to a CostId (see common.hh)
    /// The class is built with one default optimizing criterion
    DECLARE_RO_PROPERTY( optimizing_criteria, std::vector<CostId> );

    /// Write access to a particular criterion
    /// @param idx Index of the criterion (sorted by priority) must be >= 0. The first criterion is always available
    /// @param cost The CostId
    void set_optimizing_criterion( unsigned idx, const CostId& cost );

    /// Write access to a particular criterion
    /// @param idx Index of the criterion (sorted by priority) must be >= 0. The first criterion is always available
    /// @param cost The cost, as an int
    void set_optimizing_criterion( unsigned idx, int cost );

    /// Add a new criterion (with the lowest priority)
    void add_criterion( CostId criterion );

    ///
    /// Exception thrown when a request has inconsistent timing constraints
    class BadRequestTiming : public std::out_of_range
    {
    public:
        explicit BadRequestTiming() : std::out_of_range( "Bad request timing" ) {}
    };

private:
    /// check time causality
    bool check_timing_() const;
};
} // Tempus namespace

#endif
