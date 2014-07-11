/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
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

#include <iostream>
#include "progression.hh"

namespace Tempus
{

ProgressionCallback null_progression_callback;

void TextProgression::operator()( float percent, bool finished )
{
    // only update when changes are visible
    int new_N = int( percent * N_ );

    if ( new_N == old_N_ ) {
        return;
    }

    /*if ( finished || ( old_N_ == -1 ) )*/ {
        std::cout << "\r";
        std::cout << "[";

        for ( int i = 0; i < new_N; i++ ) {
            std::cout << ".";
        }

        for ( int i = new_N; i < N_; i++ ) {
            std::cout << " ";
        }

        std::cout << "] ";
        std::cout << int( percent * 100 ) << "%";

        if ( finished ) {
            std::cout << std::endl;
        }

        std::cout.flush();
    }

    old_N_ = new_N;
}

}
