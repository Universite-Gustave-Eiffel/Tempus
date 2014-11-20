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

#include "request.hh"

namespace Tempus {

void Request::TimeConstraint::type( int t )
{
    BOOST_ASSERT( t >= 0 && t <= TimeConstraint::LastValue );
    type_ = static_cast<TimeConstraintType>(t);
}

Request::Request()
{
    steps_.push_back( Step() ); // origin
    steps_.push_back( Step() ); // destination
    optimizing_criteria_.push_back( CostDistance ); // default criterion
}

Request::Request( const Step& o, const Step& d )
{
    steps_.push_back( o );
    steps_.push_back( d );
    if ( !check_timing_() ) {
        throw BadRequestTiming();
    }
    optimizing_criteria_.push_back( CostDistance ); // default criterion
}

void Request::add_intermediary_step( const Step& step )
{
    steps_.insert( steps_.end()-1, step );
    if ( !check_timing_() ) {
        throw BadRequestTiming();
    }
}

void Request::add_allowed_mode( db_id_t m )
{
    // add a new mode, make sure the resulting vector stay sorted
    allowed_modes_.push_back( m );
    std::sort( allowed_modes_.begin(), allowed_modes_.end() );
}

Road::Vertex Request::origin() const
{
    return steps_.front().location();
}

void Request::set_origin( const Road::Vertex& v )
{
    steps_[0].set_location( v );
}

void Request::set_origin( const Step& s )
{
    steps_[0] = s;
}

Road::Vertex Request::destination() const
{
    return steps_.back().location();
}

void Request::set_destination( const Road::Vertex& v )
{
    steps_.back().set_location( v );
}

void Request::set_destination( const Step& s )
{
    steps_.back() = s;
}

void Request::set_optimizing_criterion( unsigned idx, const CostId& c )
{
    BOOST_ASSERT( idx < optimizing_criteria_.size() );
    optimizing_criteria_[idx] = c;
}

void Request::set_optimizing_criterion( unsigned idx, int cost )
{
    BOOST_ASSERT( cost >= FirstValue && cost <= LastValue );
    set_optimizing_criterion( idx, static_cast<CostId>(cost) );
}

void Request::add_criterion( CostId criterion )
{
    optimizing_criteria_.push_back( criterion );
}

bool Request::check_timing_() const
{
    Step last = steps_[0];
    for ( size_t i = 1; i < steps_.size(); i++ )
    {
        if ( (steps_[i].constraint().type() != TimeConstraint::NoConstraint) &&
             (steps_[i].constraint().date_time() >= last.constraint().date_time()) ) {
            return false;
        }
        last = steps_[i];
    }
    return true;
}

} // Tempus namespace
