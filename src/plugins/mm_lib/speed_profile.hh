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

#ifndef TEMPUS_SPEED_PROFILE_HH
#define TEMPUS_SPEED_PROFILE_HH

#include "road_graph.hh"
#include "transport_modes.hh"

namespace Tempus {

struct SpeedTimePeriod
{
    // length of the period (minutes)
    double length;
    // average speed (km/h)
    double speed;
};

class RoadEdgeSpeedProfile
{
public:
    RoadEdgeSpeedProfile() {}

    // speed rule -> time of the day (in minutes) -> SpeedTimePeriod
    typedef std::map< TransportModeSpeedRule, std::map< double, SpeedTimePeriod > > SpeedProfile;
    // road edge -> speed profile
    typedef std::map< db_id_t, SpeedProfile > RoadEdgeSpeedProfileMap;

    typedef std::map<double, SpeedTimePeriod>::const_iterator PeriodIterator;

    std::pair<PeriodIterator, PeriodIterator> periods_after( db_id_t section_id, TransportModeSpeedRule speed_rule, double begin_time, bool& found ) const;

    void add_period( db_id_t section_id, TransportModeSpeedRule speed_rule, double begin_time, double length, double speed );
private:
    RoadEdgeSpeedProfileMap speed_profile_;
};

} // namespace Tempus

#endif
