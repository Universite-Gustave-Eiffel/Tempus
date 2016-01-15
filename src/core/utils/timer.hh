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

#include <chrono>

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
        t_start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    ///
    /// Get elapsed number of seconds since construction
    double elapsed()
    {
        return elapsed_ms() / 1000;
    }

    ///
    /// Get elapsed number of milliseconds since construction
    double elapsed_ms()
    {
        auto t_stop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        return double((t_stop - t_start).count());
    }
private:
    std::chrono::milliseconds t_start;
};

#endif
