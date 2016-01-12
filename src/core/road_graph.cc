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

namespace Tempus {
namespace Road {

void Restrictions::add_restriction( db_id_t id,
                                    const Restriction::EdgeSequence& edge_seq,
                                    const Restriction::CostPerTransport& cost )
{
    Restriction::EdgeSequence road_edges;
    const Road::Graph& graph = *road_graph_;

    if ( edge_seq.size() == 0 ) {
        return;
    }

    if ( edge_seq.size() == 1 ) {
        // should not happen, push back in an arbitrary order
        restrictions_.push_back( Restriction( id, edge_seq, cost ) );
        return;
    }

    Edge edge1( edge_seq[0] );
    Edge edge2( edge_seq[1] );

    // special case for u-turns : duplicate and reverse
    if ( edge_seq.size() == 2 && edge1 == edge2 ) {
        Restriction::EdgeSequence seq2;
        bool found;
        boost::tie( edge2, found ) = edge( target( edge1, graph ),
                                           source( edge1, graph ),
                                           graph );

        if ( !found ) {
            // U-turn on a one-way section ?
            return;
        }
        
        seq2.push_back( edge1 );
        seq2.push_back( edge2 );
        restrictions_.push_back( Restriction( id, seq2, cost ) );

        seq2.clear();
        seq2.push_back( edge2 );
        seq2.push_back( edge1 );
        restrictions_.push_back( Restriction( id, seq2, cost ) );
        return;
    }

    for ( size_t i = 0; i < edge_seq.size() - 1; ++i ) {
        edge1 = edge_seq[i];
        edge2 = edge_seq[i+1];

        if ( source( edge1, graph ) == source( edge2, graph ) ||
             source( edge1, graph ) == target( edge2, graph ) ) {
            edge1 = edge( target( edge1, graph ),
                          source( edge1, graph ),
                          graph ).first;
        }
        road_edges.push_back( edge1 );
    }

    if ( source( edge2, graph ) == target( edge1, graph ) ) {
        road_edges.push_back( edge2 );
    }
    else {
        edge2 = edge( target( edge2, graph ),
                              source( edge2, graph ),
                              graph ).first;
        road_edges.push_back( edge2 );
    }

    restrictions_.push_back( Restriction( id, road_edges, cost ) );
}

void Restrictions::add_restriction( Restriction r )
{
    restrictions_.push_back( r );
}

}
}

namespace boost {
namespace detail {
std::ostream& operator<<( std::ostream& ostr, const Tempus::Road::Edge& e )
{
    ostr << "(" << e.src << "+" << e.idx << ")";
    return ostr;
}
}
}


