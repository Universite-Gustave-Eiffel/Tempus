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

#ifndef TEMPUS_UTILS_TIMER_HH
#define TEMPUS_UTILS_TIMER_HH

#include <sys/timeb.h>
#include <time.h>

#ifdef _WIN32
#define ftime(x) _ftime(x)
#define timeb _timeb
#endif

///
/// RAII Timer class
class Timer
{
public:
    Timer()
    {
        // start the timer
        restart();
    }

    void restart()
    {
        ftime( &t_start );
    }

    ///
    /// Get elapsed number of seconds since construction
    double elapsed()
    {
        ftime( &t_stop );
        double sstart = t_start.time + t_start.millitm / 1000.0;
        double sstop = t_stop.time + t_stop.millitm / 1000.0;
        return sstop - sstart;
    }
private:
    timeb t_start, t_stop;
};

#endif
