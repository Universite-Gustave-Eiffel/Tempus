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

#ifndef TEMPUS_REVERSE_MULTIMODAL_GRAPH_HH
#define TEMPUS_REVERSE_MULTIMODAL_GRAPH_HH

#include <boost/graph/reverse_graph.hpp>

#include "multimodal_graph.hh"

namespace Tempus {

namespace Road {
typedef boost::reverse_graph<Road::Graph> ReverseGraph;
}

namespace PublicTransport {
typedef boost::reverse_graph<PublicTransport::Graph> ReverseGraph;
}

namespace Multimodal {

///
/// A ReverseGraph is an adaptor based on a multimodal graph
/// that reverses in and out edges

struct ReverseGraph: boost::noncopyable {
    // declaration for boost::graph
    typedef Tempus::Multimodal::Vertex          vertex_descriptor;
    typedef Tempus::Multimodal::Edge            edge_descriptor;
    // inversion here
    typedef Tempus::Multimodal::InEdgeIterator out_edge_iterator;
    typedef Tempus::Multimodal::OutEdgeIterator  in_edge_iterator;

    typedef Tempus::Multimodal::VertexIterator  vertex_iterator;
    typedef Tempus::Multimodal::EdgeIterator    edge_iterator;

    typedef boost::bidirectional_tag            directed_category;

    typedef boost::disallow_parallel_edge_tag   edge_parallel_category;
    typedef boost::bidirectional_graph_tag      traversal_category;

    typedef size_t vertices_size_type;
    typedef size_t edges_size_type;
    typedef size_t degree_size_type;

    // unused types (here to please boost::graph_traits<>)
    typedef void adjacency_iterator;

    static inline vertex_descriptor null_vertex() {
        return vertex_descriptor();   // depending on boost version, this can be useless
    }

    /// Constructor
    /// Needs an actual multimodal graph
    explicit ReverseGraph( const Multimodal::Graph& graph );

    ///
    /// The road graph
    const Road::ReverseGraph& road() const;

    ///
    /// Public transport networks
    typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;
    NetworkMap network_map() const;

    /// Access to a particular network
    boost::optional<const PublicTransport::Network&> network( db_id_t ) const;
    
    /// Access to a particular graph
    boost::optional<PublicTransport::ReverseGraph> public_transport( db_id_t ) const;

    ///
    /// Point of interests
    typedef boost::ptr_map<db_id_t, POI> PoiList;
    PoiList pois() const;

    /// Access to a particular poi
    boost::optional<const POI&> poi( db_id_t ) const;

    /// Transport modes
    typedef std::map<db_id_t, TransportMode> TransportModes;
    TransportModes transport_modes() const;

    /// access to a transportmode, given its id
    /// the second element of the pair tells if the mode exists
    boost::optional<TransportMode> transport_mode( db_id_t id ) const;

    /// access to a transportmode, given its name
    /// the second element of the pair tells if the mode exists
    boost::optional<TransportMode> transport_mode( const std::string& name ) const;

private:
    const Graph& graph_;
    Road::ReverseGraph rroad_;

public:
    /// Access to the underlying graph
    const Graph& graph() const {
        return graph_;
    }
};

//
// Boost graph functions declarations
// A Multimodal::Graph is an IncidenceGraph and a VertexAndEdgeListGraph

///
/// Number of vertices. Constant time
size_t num_vertices( const ReverseGraph& graph );
///
/// Number of edges. O(n) for n number of PT and POI edges
size_t num_edges( const ReverseGraph& graph );
///
/// Returns source vertex from an edge. Constant time (linear in number of PT networks)
Vertex source( const Edge& e, const ReverseGraph& graph );
///
/// Returns source vertex from an edge. Constant time (linear in number of PT networks)
Vertex target( const Edge& e, const ReverseGraph& graph );

///
/// Returns a range of VertexIterator. Constant time
std::pair<VertexIterator, VertexIterator> vertices( const ReverseGraph& graph );
///
/// Returns a range of EdgeIterator. Constant time
std::pair<EdgeIterator, EdgeIterator> edges( const ReverseGraph& graph );
///
/// Returns a range of EdgeIterator that allows to iterate on out edges of a vertex. Constant time
std::pair<InEdgeIterator, InEdgeIterator> out_edges( const Vertex& v, const ReverseGraph& graph );
///
/// Returns a range of EdgeIterator that allows to iterate on in edges of a vertex. Constant time
std::pair<OutEdgeIterator, OutEdgeIterator> in_edges( const Vertex& v, const ReverseGraph& graph );
///
/// Number of out edges for a vertex.
size_t out_degree( const Vertex& v, const ReverseGraph& graph );
///
/// Number of in edges for a vertex.
size_t in_degree( const Vertex& v, const ReverseGraph& graph );
///
/// Number of out and in edges for a vertex.
size_t degree( const Vertex& v, const ReverseGraph& graph );

///
/// Find an edge, based on a source and target vertex.
/// It does not implements AdjacencyMatrix, since it does not returns in constant time (linear in the number of edges)
std::pair< Edge, bool > edge( const Vertex& u, const Vertex& v, const ReverseGraph& graph );

///
/// Overloading of get()
VertexIndexProperty get( boost::vertex_index_t, const Multimodal::ReverseGraph& graph );

}
}

#endif
