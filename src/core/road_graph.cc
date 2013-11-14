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

#include "road_graph.hh"

namespace Tempus
{
    namespace Road
    {
        Road::VertexSequence Road::to_vertex_sequence( const Graph& graph ) const
        {
            Road::VertexSequence seq;
            if ( road_sections.size() == 0 ) {
                return seq;
            }
            if ( road_sections.size() == 1 ) {
                // should not happen, push back in an arbitrary order
                Edge edge( road_sections[0] );
                seq.push_back( source( edge, graph ) );
                seq.push_back( target( edge, graph ) );
                return seq;
            }

            Edge edge1( road_sections[0] );
            Edge edge2( road_sections[1] );
            if ( (target( edge1, graph ) == source( edge2, graph )) ||
                 (target( edge1, graph ) == target( edge2, graph )) ) {
                seq.push_back( source( edge1, graph ) );
                seq.push_back( target( edge1, graph ) );
            }
            else if ( (source( edge1, graph ) == source( edge2, graph )) ||
                      (source( edge1, graph ) == target( edge2, graph )) ) {
                seq.push_back( target( edge1, graph ) );
                seq.push_back( source( edge1, graph ) );
            }

            for ( size_t i = 1; i < road_sections.size(); ++i ) {
                Edge edge( road_sections[i] );
                if ( source( edge, graph ) == seq.back() ) {
                    seq.push_back( target( edge, graph ) );
                }
                else {
                    seq.push_back( source( edge, graph ) );
                }
            }
            return seq;
        }
    }
}
