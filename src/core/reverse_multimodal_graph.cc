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

#include "reverse_multimodal_graph.hh"

namespace Tempus {

namespace Multimodal {

ReverseGraph::ReverseGraph( const Graph& g ) : graph_(g), rroad_(g.road()) {
}

const Road::Graph& ReverseGraph::road() const
{
    return graph_.road();
}

const Road::ReverseGraph& ReverseGraph::reverse_road() const
{
    return rroad_;
}

ReverseGraph::NetworkMap ReverseGraph::network_map() const
{
    return graph_.network_map();
}

boost::optional<const PublicTransport::Network&> ReverseGraph::network( db_id_t id ) const
{
    return graph_.network( id );
}

boost::optional<PublicTransport::Graph> ReverseGraph::public_transport( db_id_t id ) const
{
    return graph_.public_transport(id).get();
}

boost::optional<PublicTransport::ReverseGraph> ReverseGraph::reverse_public_transport( db_id_t id ) const
{
    return PublicTransport::ReverseGraph( graph_.public_transport(id).get() );
}

ReverseGraph::PoiList ReverseGraph::pois() const
{
    return graph_.pois();
}

ReverseGraph::TransportModes ReverseGraph::transport_modes() const
{
    return graph_.transport_modes();
}

boost::optional<TransportMode> ReverseGraph::transport_mode( db_id_t id ) const
{
    return graph_.transport_mode( id );
}

boost::optional<TransportMode> ReverseGraph::transport_mode( const std::string& name ) const
{
    return graph_.transport_mode( name );
}

size_t num_vertices( const ReverseGraph& graph )
{
    return num_vertices( graph.graph() );
}
size_t num_edges( const ReverseGraph& graph )
{
    return num_edges( graph.graph() );
}
Vertex source( const Edge& e, const ReverseGraph& graph )
{
    return target( e, graph.graph() );
}
Vertex target( const Edge& e, const ReverseGraph& graph )
{
    return source( e, graph.graph() );
}
std::pair<VertexIterator, VertexIterator> vertices( const ReverseGraph& graph )
{
    return vertices( graph.graph() );
}
std::pair<EdgeIterator, EdgeIterator> edges( const ReverseGraph& graph )
{
    return edges( graph.graph() );
}
std::pair<InEdgeIterator, InEdgeIterator> out_edges( const Vertex& v, const ReverseGraph& graph )
{
    // inversion
    return in_edges( v, graph.graph() );
}
std::pair<OutEdgeIterator, OutEdgeIterator> in_edges( const Vertex& v, const ReverseGraph& graph )
{
    // inversion
    return out_edges( v, graph.graph() );
}
size_t out_degree( const Vertex& v, const ReverseGraph& graph )
{
    // inversion
    return in_degree( v, graph.graph() );
}
size_t in_degree( const Vertex& v, const ReverseGraph& graph )
{
    // inversion
    return out_degree( v, graph.graph() );
}
size_t degree( const Vertex& v, const ReverseGraph& graph )
{
    return degree( v, graph.graph() );
}

std::pair< Edge, bool > edge( const Vertex& u, const Vertex& v, const ReverseGraph& graph )
{
    return edge( u, v, graph.graph() );
}

VertexIndexProperty get( boost::vertex_index_t t, const Multimodal::ReverseGraph& graph )
{
    return get( t, graph.graph() );
}

}
}
