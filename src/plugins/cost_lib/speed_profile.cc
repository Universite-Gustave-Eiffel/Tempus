/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "speed_profile.hh"

namespace Tempus {

void RoadEdgeSpeedProfile::add_period( db_id_t section_id, TransportModeSpeedRule speed_rule, double begin_time, double length, double speed )
{
    bool found = false;
    RoadEdgeSpeedProfileMap::iterator it;
    boost::tie(it, found) = speed_profile_.insert( std::make_pair(section_id, SpeedProfile()) );
    SpeedProfile::iterator sit;
    boost::tie(sit, found) = it->second.insert( std::make_pair(speed_rule, std::map<double, SpeedTimePeriod>()) );
    std::map<double, SpeedTimePeriod>::iterator tit;
    boost::tie(tit, found) = sit->second.insert( std::make_pair(begin_time, SpeedTimePeriod()) );
    tit->second.length = length;
    tit->second.speed = speed;
}

std::pair<RoadEdgeSpeedProfile::PeriodIterator, RoadEdgeSpeedProfile::PeriodIterator> RoadEdgeSpeedProfile::periods_after( db_id_t section_id, TransportModeSpeedRule speed_rule, double begin_time, bool& found ) const
{
    RoadEdgeSpeedProfileMap::const_iterator sit = speed_profile_.find( section_id );
    if ( sit != speed_profile_.end() ) {
        SpeedProfile::const_iterator it = sit->second.find( speed_rule );
        if ( it != sit->second.end() ) {
            std::map<double, SpeedTimePeriod>::const_iterator mit = it->second.upper_bound( begin_time );
            // we suppose we cover 24h of speed profile
            BOOST_ASSERT( mit != it->second.end() );
            mit--;
			found = true;
            return std::make_pair( mit, it->second.end() );
        }
    }
	// inconsistent return value
    return std::make_pair( PeriodIterator(), PeriodIterator() );
}

}
