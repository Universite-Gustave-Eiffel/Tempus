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

#ifndef TEMPUS_ROADMAP_HH
#define TEMPUS_ROADMAP_HH

#include <map>
#include <boost/ptr_container/ptr_vector.hpp>

#include "common.hh"
#include "multimodal_graph.hh"

namespace Tempus {
/**
   A Roadmap is an object used to model steps involved in a multimodal route.
   It is a base for result values of a request.
*/
class Roadmap {
public:
    ///
    /// A Step is a part of a route, where the transport type is constant
    /// This a generic class
    class Step {
    public:
        enum StepType {
            RoadStep,
            PublicTransportStep,
            GenericStep
        };
        DECLARE_RW_PROPERTY( step_type, StepType );

        DECLARE_RO_PROPERTY( costs, Costs );
        /// Gets a cost
        double cost( CostId id ) const;
        /// Sets a cost
        void cost( CostId id, double c );

        /// (Initial) transport mode id
        DECLARE_RW_PROPERTY( transport_mode, db_id_t );

        /// Geometry of the step, described as a WKB, for visualization purpose
        /// May be empty.
        DECLARE_RW_PROPERTY( geometry_wkb, std::string );

        Step( StepType type ) : step_type_( type ) {}

        virtual Step* clone() const {
            return new Step( *this );
        }
    };

    ///
    /// A Step that occurs on the road, either by a pedestrian or a private vehicle
    struct RoadStep : public Step {
        RoadStep() : Step( Step::RoadStep ) {}

        ///
        /// The road section where to start from
        DECLARE_RW_PROPERTY( road_edge, Road::Edge );

        ///
        /// The road name (field not present in a Road::Section, so copied from the DB here)
        DECLARE_RW_PROPERTY( road_name, std::string );

        ///
        /// Distance to walk/drive (in km). -1 if we have to go until the end of the section
        ///
        DECLARE_RW_PROPERTY( distance_km, double );

        /// The movement that is to be done at the end of the section
        enum EndMovement {
            GoAhead,
            TurnLeft,
            TurnRight,
            UTurn,
            RoundAboutEnter,
            FirstExit, ///< in a roundabout
            SecondExit,
            ThirdExit,
            FourthExit,
            FifthExit,
            SixthExit,
            YouAreArrived = 999
        };
        DECLARE_RW_PROPERTY( end_movement, EndMovement );

        virtual RoadStep* clone() const {
            return new RoadStep( *this );
        }
    };

    ///
    /// A Step made with a public transport
    ///
    /// For a trip from station A to station C that passes through the station B,
    /// 2 steps are stored, each with the same trip_id.
    struct PublicTransportStep : public Step {
        PublicTransportStep() : Step( Step::PublicTransportStep ), wait(0.0) {}

        db_id_t network_id;

        ///
        /// Wait time at this step
        double wait; 

        ///
        /// The public transport section part of the step
        PublicTransport::Edge section;

        db_id_t trip_id; ///< used to indicate the direction

        virtual PublicTransportStep* clone() const {
            return new PublicTransportStep( *this );
        }
    };

    ///
    /// A generic step from a vertex to another
    /// Inherits from Step as well as from Multimodal::Edge
    /// This is used to represent a step from a mode to another (road, public transport, poi, etc)
    struct GenericStep : public Step, public Multimodal::Edge {
        GenericStep() : Step( Step::GenericStep ), Edge() {}
        GenericStep( const Multimodal::Edge& edge ) : Step( Step::GenericStep ), Edge( edge ) {}

        ///
        /// Road name, if relevant
        DECLARE_RW_PROPERTY( road_name, std::string );

        /// Final transport mode id
        DECLARE_RW_PROPERTY( final_mode, db_id_t );

        virtual GenericStep* clone() const {
            return new GenericStep( *this );
        }
    };

    ///
    /// A Roadmap is a list of Step augmented with some total costs.
    typedef boost::ptr_vector<Step> StepList;
    StepList steps;

    // FIXME replace by a compute-only method ?
    DECLARE_RO_PROPERTY( total_costs, Costs );

    /// Get a particular cost
    double total_cost( CostId id ) const;
    /// Set a total cost
    void total_cost( CostId id, double c );
};

///
/// A Result is a list of Roadmap, ordered by relevance towards optimizing criteria
typedef std::list<Roadmap> Result;

///
/// Used internally by boost::ptr_vector when copying
inline Roadmap::Step* new_clone( const Roadmap::Step& step )
{
    return step.clone();
}

} // Tempus namespace

#endif
