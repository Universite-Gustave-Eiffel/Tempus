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

#ifndef TEMPUS_PROGRESSION_HH
#define TEMPUS_PROGRESSION_HH

namespace Tempus
{
///
/// Base class in charge of progression callback.
///
/// This is used for methods that might take time before giving user feedback
/// See pgsql_importer.hh for instance
class ProgressionCallback {
public:
    virtual void operator()( float, bool = false ) {
        // Default : do nothing
    }
};

///
/// The default (null) progression callback that does nothing
extern ProgressionCallback null_progression_callback;


///
/// Simple progession processing: text based progression bar.
struct TextProgression : public Tempus::ProgressionCallback {
public:
    TextProgression( int N = 50 ) : N_( N ), old_N_( -1 ) {
    }
    virtual void operator()( float percent, bool finished );
protected:
    int N_;
    int old_N_;
};

}

#endif
