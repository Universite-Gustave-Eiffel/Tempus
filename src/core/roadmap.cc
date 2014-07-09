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

#include "roadmap.hh"
#include <boost/assert.hpp>

namespace Tempus {

double Roadmap::Step::cost( CostId id ) const
{
    Costs::const_iterator it = costs_.find(id);
    BOOST_ASSERT( it != costs_.end() );
    return it->second;
}

void Roadmap::Step::set_cost( CostId id, double c )
{
    costs_[id] = c;
}

Costs get_total_costs( const Roadmap& roadmap )
{
    Costs costs;
    for ( Roadmap::StepConstIterator it = roadmap.begin(); it != roadmap.end(); ++it ) {
        for ( Costs::const_iterator cit = it->costs().begin(); cit != it->costs().end(); ++cit ) {
            if ( costs.find(cit->first) == costs.end() ) {
                costs[cit->first] = cit->second;
            }
            else {
                costs[cit->first] += cit->second;
            }
        }
    }
    return costs;
}

Roadmap::StepConstIterator Roadmap::begin() const
{
    return steps_.begin();
}

Roadmap::StepIterator Roadmap::begin()
{
    return steps_.begin();
}

Roadmap::StepConstIterator Roadmap::end() const
{
    return steps_.end();
}

Roadmap::StepIterator Roadmap::end()
{
    return steps_.end();
}

const Roadmap::Step& Roadmap::step( size_t idx ) const
{
    return steps_.at(idx);
}

void Roadmap::add_step( std::auto_ptr<Step> s )
{
    steps_.push_back( s.release() );
}

}
