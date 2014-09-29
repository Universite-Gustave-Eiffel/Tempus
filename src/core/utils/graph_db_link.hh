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

// Utility functions that make a link between graph indexes and DB ids

#include <boost/graph/graph_traits.hpp>

#include "../common.hh"
#include "../road_graph.hh"
#include "../public_transport_graph.hh"
#include "../multimodal_graph.hh"
#include "../db.hh"
#include "../point.hh"

namespace Tempus {
///
/// Get a vertex descriptor from its database's id.
/// This is templated in a way that it is compliant with Road::Vertex, PublicTransport::Vertex
template <class G>
std::pair<typename boost::graph_traits<G>::vertex_descriptor, bool> vertex_from_id( Tempus::db_id_t db_id, G& graph )
{
    typename boost::graph_traits<G>::vertex_iterator vi, vi_end;

    for ( boost::tie( vi, vi_end ) = vertices( graph ); vi != vi_end; vi++ ) {
        if ( graph[*vi].db_id() == db_id ) {
            return std::make_pair( *vi, true );
        }
    }

    // null element
    return std::make_pair( typename boost::graph_traits<G>::vertex_descriptor(), false );
}

///
/// Get an edge descriptor from its database's id.
/// This is templated in a way that it is compliant with Road::Edge
/// A PublicTransport::Edge has no unique id associated.
template <class G>
std::pair< typename boost::graph_traits<G>::edge_descriptor, bool > edge_from_id( Tempus::db_id_t db_id, G& graph )
{
    typename boost::graph_traits<G>::edge_iterator vi, vi_end;

    for ( boost::tie( vi, vi_end ) = edges( graph ); vi != vi_end; vi++ ) {
        if ( graph[*vi].db_id() == db_id ) {
            return std::make_pair( *vi, true );
        }
    }

    // null element
    return std::make_pair( typename boost::graph_traits<G>::edge_descriptor(), false );
}

///
/// Get 2D coordinates of a road vertex, from the database
Point2D coordinates( const Road::Vertex& v, Db::Connection& db, const Road::Graph& graph );
///
/// Get 2D coordinates of a public transport vertex, from the database
Point2D coordinates( const PublicTransport::Vertex& v, Db::Connection& db, const PublicTransport::Graph& graph );
///
/// Get 2D coordinates of a POI, from the database
Point2D coordinates( const POI* poi, Db::Connection& db );
///
/// Get 2D coordinates of a multimodal vertex, from the database
Point2D coordinates( const Multimodal::Vertex& v, Db::Connection& db, const Multimodal::Graph& graph );
}
