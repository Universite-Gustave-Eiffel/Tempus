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

#include "multimodal_graph.hh"

namespace Tempus {

///
/// Reverse graph adaptor (read only) for Road and Public Transport
/// This is inspired by boost::reverse_graph
/// But this one does not create a new type for vertex / edge descriptors for the reverse graph
/// which simplifies things
template <class OGraph >
class ReverseGraphT
{
public:
    explicit ReverseGraphT( const OGraph& g ) : graph_(g) {}

    typedef typename OGraph::vertex_property_type Node;
    typedef typename OGraph::edge_property_type Section;

    typedef typename boost::graph_traits<OGraph>::vertex_descriptor vertex_descriptor;
    typedef typename boost::graph_traits<OGraph>::vertex_descriptor Vertex;
    typedef typename boost::graph_traits<OGraph>::edge_descriptor edge_descriptor;
    typedef typename boost::graph_traits<OGraph>::edge_descriptor Edge;
    // inversion
    typedef typename boost::graph_traits<OGraph>::in_edge_iterator out_edge_iterator;
    typedef typename boost::graph_traits<OGraph>::out_edge_iterator in_edge_iterator;

    typedef typename boost::graph_traits<OGraph>::vertex_iterator vertex_iterator;
    typedef typename boost::graph_traits<OGraph>::edge_iterator edge_iterator;

    typedef boost::bidirectional_tag directed_category;

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

    const OGraph& graph() const { return graph_; }

    const Node& operator[]( Vertex v ) const {
        return graph_[v];
    }
    const Section& operator[]( Edge e ) const {
        return graph_[e];
    }

private:
    const OGraph& graph_;
};

template <class G>
size_t num_vertices( const ReverseGraphT<G>& graph )
{
    return num_vertices( graph.graph() );
}
template <class G>
size_t num_edges( const ReverseGraphT<G>& graph )
{
    return num_edges( graph.graph() );
}
template <class G>
typename ReverseGraphT<G>::Vertex source( const typename ReverseGraphT<G>::Edge& e, const ReverseGraphT<G>& graph )
{
    return target( e, graph.graph() );
}
template <class G>
typename ReverseGraphT<G>::Vertex target( const typename ReverseGraphT<G>::Edge& e, const ReverseGraphT<G>& graph )
{
    return source( e, graph.graph() );
}
template <class G>
std::pair<typename ReverseGraphT<G>::vertex_iterator,
          typename ReverseGraphT<G>::vertex_iterator>
vertices( const ReverseGraphT<G>& graph )
{
    return vertices( graph.graph() );
}
template <class G>
std::pair<typename ReverseGraphT<G>::edge_iterator,
          typename ReverseGraphT<G>::edge_iterator>
edges( const ReverseGraphT<G>& graph )
{
    return edges( graph.graph() );
}
template <class G>
std::pair<typename ReverseGraphT<G>::out_edge_iterator,
          typename ReverseGraphT<G>::out_edge_iterator>
out_edges( const typename ReverseGraphT<G>::Vertex& v, const ReverseGraphT<G>& graph )
{
    // inversion
    return in_edges( v, graph.graph() );
}
template <class G>
std::pair<typename ReverseGraphT<G>::in_edge_iterator,
          typename ReverseGraphT<G>::in_edge_iterator>
in_edges( const typename ReverseGraphT<G>::Vertex& v, const ReverseGraphT<G>& graph )
{
    // inversion
    return out_edges( v, graph.graph() );
}
template <class G>
size_t out_degree( const typename ReverseGraphT<G>::Vertex& v, const ReverseGraphT<G>& graph )
{
    // inversion
    return in_degree( v, graph.graph() );
}
template <class G>
size_t in_degree( const typename ReverseGraphT<G>::Vertex& v, const ReverseGraphT<G>& graph )
{
    // inversion
    return out_degree( v, graph.graph() );
}
template <class G>
size_t degree( const typename ReverseGraphT<G>::Vertex& v, const ReverseGraphT<G>& graph )
{
    return degree( v, graph.graph() );
}

namespace Road {
typedef ReverseGraphT<Road::Graph> ReverseGraph;
}

namespace PublicTransport {
typedef ReverseGraphT<PublicTransport::Graph> ReverseGraph;
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
    const Road::Graph& road() const;
    const Road::ReverseGraph& reverse_road() const;

    ///
    /// Public transport networks
    typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;
    NetworkMap network_map() const;

    /// Access to a particular network
    boost::optional<const PublicTransport::Network&> network( db_id_t ) const;
    
    /// Access to a particular graph
    boost::optional<PublicTransport::Graph> public_transport( db_id_t ) const;
    boost::optional<PublicTransport::ReverseGraph> reverse_public_transport( db_id_t ) const;

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

} // multimodal

template <class G>
struct is_graph_reversed
{
    static const bool value = false;
};
template <>
struct is_graph_reversed<Multimodal::ReverseGraph>
{
    static const bool value = true;
};
template <>
struct is_graph_reversed<Road::ReverseGraph>
{
    static const bool value = true;
};
template <>
struct is_graph_reversed<PublicTransport::ReverseGraph>
{
    static const bool value = true;
};

}

#endif
