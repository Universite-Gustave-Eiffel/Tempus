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

#ifndef TEMPUS_MULTIMODAL_GRAPH_HH
#define TEMPUS_MULTIMODAL_GRAPH_HH

#include <boost/variant.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "common.hh"
#include "transport_modes.hh"
#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "poi.hh"
#include "sub_map.hh"

namespace Db {
class Connection;
}

namespace Tempus {
// forward declarations
namespace Multimodal {
class VertexIterator;
class OutEdgeIterator;
class InEdgeIterator;
class EdgeIterator;

// For debugging purposes
std::ostream& operator<<( std::ostream& ostr, const VertexIterator& it );
std::ostream& operator<<( std::ostream& ostr, const OutEdgeIterator& it );
std::ostream& operator<<( std::ostream& ostr, const InEdgeIterator& it );
std::ostream& operator<<( std::ostream& ostr, const EdgeIterator& it );
}
}

namespace Tempus {
///
/// Multimodal namespace
///
/// A Multimodal::Graph is a Road::Graph, a list of PublicTransport::Graph and a list of POIs
///
namespace Multimodal {

class VertexIndexProperty;
///
/// A Multimodal::Vertex is either a Road::Vertex or PublicTransport::Vertex on a particular public transport network
struct Vertex {
public:
    ///
    /// Comparison operator
    bool operator==( const Vertex& v ) const;
    bool operator!=( const Vertex& v ) const;
    bool operator<( const Vertex& v ) const;

    /// A null vertex
    Vertex();
    /// A road vertex
    Vertex( const Road::Graph* graph, Road::Vertex vertex );
    /// A public transport vertex
    Vertex( const PublicTransport::Graph* graph, PublicTransport::Vertex vertex );
    /// A POI vertex
    explicit Vertex( const POI* poi );

    enum VertexType {
        Road = 0,        /// This vertex is a road vertex
        PublicTransport, /// This vertex is a public transport stop
        Poi              /// This vertex is a POI
    };
    VertexType type() const;

    bool is_null() const;

    /// Access to the underlying road graph
    /// @returns the road graph or 0 if it's not a road vertex
    const Road::Graph* road_graph() const;
    /// @returns the road vertex on the road graph if it's a road vertex
    Road::Vertex road_vertex() const;

    /// @returns the public transport graph or 0 if it's not a pt vertex
    const PublicTransport::Graph* pt_graph() const;
    /// @returns the pt vertex on the pt graph if it's a pt vertex
    PublicTransport::Vertex pt_vertex() const;

    /// @returns the POI, if it's a POI, or 0
    const POI* poi() const;

    /// @returns the coordinates, whatever the vertex type
    Point3D coordinates() const;

private:
    struct RoadVertex_
    {
        const Road::Graph* graph;
        Road::Vertex vertex;
        RoadVertex_() : graph(0) {}
        RoadVertex_( const Road::Graph* g, const Road::Vertex& v ) : graph(g), vertex(v) {}
        bool operator==( const RoadVertex_& other ) const {
            return graph == other.graph && vertex == other.vertex;
        }
        bool operator< (const RoadVertex_& other ) const {
            return graph == other.graph ? vertex < other.vertex : graph < other.graph;
        }
    };
    struct PtVertex_
    {
        const PublicTransport::Graph* graph;
        PublicTransport::Vertex vertex;
        PtVertex_() : graph(0) {}
        PtVertex_( const PublicTransport::Graph* g, const PublicTransport::Vertex& v) : graph(g), vertex(v) {}
        bool operator==( const PtVertex_& other ) const {
            return graph == other.graph && vertex == other.vertex;
        }
        bool operator< (const PtVertex_& other ) const {
            return graph == other.graph ? vertex < other.vertex : graph < other.graph;
        }
    };

    template <class T, class Visitor>
    struct ProxyVisitor : public boost::static_visitor<T>
    {
        Visitor visitor_;
        ProxyVisitor( Visitor visitor ) : visitor_(visitor) {}
        T operator()(const RoadVertex_& r ) const
        {
            return visitor_( *r.graph, r.vertex );
        }
        T operator()(const PtVertex_& r ) const
        {
            return visitor_( *r.graph, r.vertex );
        }
        T operator()(const POI* p ) const
        {
            return visitor_( *p );
        }
    };

    template <class T, class Visitor>
    T apply_visitor_( Visitor v ) const
    {
        ProxyVisitor<T,Visitor> pv(v);
        return boost::apply_visitor( pv, union_ );
    }

    bool is_null_;
    boost::variant< RoadVertex_, PtVertex_, const POI * > union_;

    friend class VertexIndexProperty;
};

/// Convenience function - get a Road::Node out of a Vertex, if defined
/// @returns the corresponding road node
/// warning no check is done, could crash
Road::Node get_road_node( const Vertex& v );

/// Convenience function - get a PublicTransport::Stop out of a Vertex, if defined
/// @returns the corresponding public transport stop
/// warning no check is done, could crash
PublicTransport::Stop get_pt_stop( const Vertex& v );

///
/// A multimodal edge is defined with :
/// * a source vertex
/// * a destination vertex
/// * and an orientation for road edges
struct Edge {
    ///
    /// The source vertex
    DECLARE_RO_PROPERTY( source, Multimodal::Vertex );
    ///
    /// The target vertex
    DECLARE_RO_PROPERTY( target, Multimodal::Vertex );

    ///
    /// The (oriented) road edge involved
    DECLARE_RO_PROPERTY( road_edge, Road::Edge );

    enum ConnectionType {
        UnknownConnection,
        Road2Road,
        Road2Transport,
        Transport2Road,
        Transport2Transport,
        Road2Poi,
        Poi2Road
    };
    ///
    /// Get the connection type of the edge
    ConnectionType connection_type() const;

    ///
    /// Allowed traffic rules
    unsigned traffic_rules() const;

    /// empty constructor
    Edge() {}
    /// Generic constructor
    /// Warning try to call more specialized, faster constructors
    Edge( const Multimodal::Vertex& s, const Multimodal::Vertex& t );

    /// Road2Road constructor
    Edge( const Road::Graph* graph, const Road::Vertex& s, const Road::Vertex& t );
    /// Transport2Road constructor
    Edge( const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& s,
          const Road::Graph* graph, const Road::Vertex& t );
    /// Road2Transport constructor
    Edge( const Road::Graph* graph, const Road::Vertex& s,
          const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& t );
    /// Transport2Transport constructor
    Edge( const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& s, const PublicTransport::Vertex& t );
    /// Road2Poi constructor
    Edge( const Road::Graph* graph, const Road::Vertex& s, const POI* t );
    /// Poi2Road constructor
    Edge( const POI* s, const Road::Graph* graph, const Road::Vertex& t );

    bool operator==( const Multimodal::Edge& e ) const;
    bool operator!=( const Multimodal::Edge& e ) const;
    bool operator<( const Multimodal::Edge& e ) const;
};

///
/// Get the public transport edge if the given edge is a Transport2Transport
/// else, return false
std::pair< PublicTransport::Edge, bool > public_transport_edge( const Multimodal::Edge& e );

///
/// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
struct Graph: boost::noncopyable {
    // declaration for boost::graph
    typedef Tempus::Multimodal::Vertex          vertex_descriptor;
    typedef Tempus::Multimodal::Edge            edge_descriptor;
    typedef Tempus::Multimodal::OutEdgeIterator out_edge_iterator;
    typedef Tempus::Multimodal::InEdgeIterator  in_edge_iterator;
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

    /// Cosntructor
    /// Needs a road graph at a minimum
    explicit Graph( std::auto_ptr<Road::Graph> );

    ~Graph();

    ///
    /// The road graph
    const Road::Graph& road() const;

    /// Write access
    Road::Graph& road();
private:
    std::auto_ptr<Road::Graph> road_;
public:
    void set_road( std::auto_ptr<Road::Graph> );

    ///
    /// Public transport networks
    typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;
    DECLARE_RW_PROPERTY( network_map, NetworkMap );

    /// Access to a particular network
    boost::optional<const PublicTransport::Network&> network( db_id_t ) const;
    
    ///
    /// Public transports graphs
    /// network_id -> PublicTransport::Graph
    /// This a sub_map that can thus be filtered to select only a subset
    /// Public transport graphs are owned by this class
    /// FIXME find a smart pointer class that work with sub_map
    typedef sub_map< db_id_t, const PublicTransport::Graph* > PublicTransportGraphList;
    DECLARE_RO_PROPERTY( public_transports, PublicTransportGraphList );

    /// Access to a particular graph
    boost::optional<const PublicTransport::Graph&> public_transport( db_id_t ) const;
    /// take ownership (move)
    void set_public_transports( boost::ptr_map<db_id_t, PublicTransport::Graph>& );

    /// Select public transports
    void select_public_transports( const std::set<db_id_t>& );
    /// Current public transport selection
    std::set<db_id_t> public_transport_selection() const;

    ///
    /// Point of interests
    typedef boost::ptr_map<db_id_t, POI> PoiList;
    DECLARE_RO_PROPERTY( pois, PoiList );
    /// Access to a particular poi
    boost::optional<const POI&> poi( db_id_t ) const;
    /// Set POIs (take ownership)
    void set_pois( boost::ptr_map<db_id_t, POI>& );

    typedef std::map<db_id_t, TransportMode> TransportModes;
    DECLARE_RO_PROPERTY( transport_modes, TransportModes );

    void set_transport_modes( const TransportModes& );

    /// access to a transportmode, given its id
    /// the second element of the pair tells if the mode exists
    boost::optional<TransportMode> transport_mode( db_id_t id ) const;

    /// access to a transportmode, given its name
    /// the second element of the pair tells if the mode exists
    boost::optional<TransportMode> transport_mode( const std::string& name ) const;

private:
    typedef std::map<std::string, Tempus::db_id_t> NameToId;
    ///
    /// Associative array that maps a transport type name to a transport type id
    NameToId transport_mode_from_name_;
};

///
/// Class that implements the Iterator concept for vertices of a Multimodal::Graph
///
/// It is a wrapper around:
///  - a vertex iterator on the current road graph
///  - an iterator on the public networks
///  - a vertex iterator on the current public network
///
/// Deferencing, incrementation and comparison operators are defined by means of these underlying iterators
///
class VertexIterator :
    public boost::iterator_facade< VertexIterator,
    Vertex,
    boost::forward_traversal_tag,
/* reference */ Vertex>

{
public:
    VertexIterator() : graph_( 0 ) {}
    VertexIterator( const Graph& graph );

    ///
    /// Move the iterator to the end. Used mainly by vertices( const Multimodal::Graph& )
    void to_end();

    ///
    /// Dereferencing. Needed by boost::iterator_facade
    Vertex dereference() const;

    ///
    /// Incrementing. Needed by boost::iterator_facade
    void increment();

    ///
    /// Comparison operator. Needed by boost::iterator_facade
    bool equal( const VertexIterator& v ) const;

protected:
    Road::VertexIterator road_it_, road_it_end_;
    Multimodal::Graph::PublicTransportGraphList::const_subset_iterator pt_graph_it_, pt_graph_it_end_;
    Multimodal::Graph::PoiList::const_iterator poi_it_, poi_it_end_;
    PublicTransport::VertexIterator pt_it_, pt_it_end_;
    const Multimodal::Graph* graph_;

    friend std::ostream& operator<<( std::ostream& ostr, const Multimodal::VertexIterator& it );
};

///
/// Class that implements the out edges iterator concept of a Multimodal::Graph
///
/// It is a wrapper around:
///  - a source vertex
///  - a road edge iterator for road edges
///  - a public transport edge iterator
///  - various counters to deal with road <-> transport stops and road <-> poi
///
/// Deferencing, incrementation and comparison operators are defined by means of these underlying iterators
///
class OutEdgeIterator :
    public boost::iterator_facade< OutEdgeIterator,
    Multimodal::Edge,
    boost::forward_traversal_tag,
/* reference */ Multimodal::Edge> {
public:
    OutEdgeIterator() : graph_( 0 ) {}
    OutEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source );

    void to_end();
    Multimodal::Edge dereference() const;
    void increment();
    bool equal( const OutEdgeIterator& v ) const;
protected:
    ///
    /// The source vertex
    Multimodal::Vertex source_;

    ///
    /// The underlying graph
    const Multimodal::Graph* graph_;

    ///
    /// A pair of out edge iterators for road vertices
    Road::OutEdgeIterator road_it_, road_it_end_;

    ///
    /// A pair of out edge iterators for public transport vertices
    PublicTransport::OutEdgeIterator pt_it_, pt_it_end_;

    ///
    /// A counter used to represent position on a Transport2Road connection.
    /// Indeed, a transport stop is linked to a road section and thus to 2 road nodes.
    /// 0: on the node_from of the associated road section
    /// 1: on the node_to
    /// 2: out of the connection
    size_t stop2road_connection_;

    ///
    /// A counter used to represent position on a Road2Transport connection.
    /// A road node can be linked to 0..N public transport nodes (@relates Road::Section::stops)
    int road2stop_connection_;

    ///
    /// A counter used to represent position on a Road2Poi connection.
    /// A road node can be linked to 0..N POI (@relates Road::Section::pois)
    int road2poi_connection_;

    ///
    /// A counter used to represent position on a Poi2Road connection.
    /// Indeed, a POI is linked to a road section and thus to 2 road nodes.
    /// 0: on the node_from of the associated road section
    /// 1: on the node_to
    /// 2: out of the connection
    int poi2road_connection_;

    friend std::ostream& operator<<( std::ostream& ostr, const OutEdgeIterator& it );
};

class InEdgeIterator :
    public boost::iterator_facade< InEdgeIterator,
    Multimodal::Edge,
    boost::forward_traversal_tag,
/* reference */ Multimodal::Edge> {
public:
    InEdgeIterator() : graph_( 0 ) {}
    InEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source );

    void to_end();
    Multimodal::Edge dereference() const;
    void increment();
    bool equal( const InEdgeIterator& v ) const;
protected:
    ///
    /// The source vertex
    Multimodal::Vertex source_;

    ///
    /// The underlying graph
    const Multimodal::Graph* graph_;

    ///
    /// A pair of out edge iterators for road vertices
    Road::InEdgeIterator road_it_, road_it_end_;

    ///
    /// A pair of out edge iterators for public transport vertices
    PublicTransport::InEdgeIterator pt_it_, pt_it_end_;

    ///
    /// A counter used to represent position on a Transport2Road connection.
    /// Indeed, a transport stop is linked to a road section and thus to 2 road nodes.
    /// 0: on the node_from of the associated road section
    /// 1: on the node_to
    /// 2: out of the connection
    size_t stop_from_road_connection_;

    ///
    /// A counter used to represent position on a Road2Transport connection.
    /// A road node can be linked to 0..N public transport nodes (@relates Road::Section::stops)
    int road_from_stop_connection_;

    ///
    /// A counter used to represent position on a Road2Poi connection.
    /// A road node can be linked to 0..N POI (@relates Road::Section::pois)
    int road_from_poi_connection_;

    ///
    /// A counter used to represent position on a Poi2Road connection.
    /// Indeed, a POI is linked to a road section and thus to 2 road nodes.
    /// 0: on the node_from of the associated road section
    /// 1: on the node_to
    /// 2: out of the connection
    int poi_from_road_connection_;

    friend std::ostream& operator<<( std::ostream& ostr, const InEdgeIterator& it );
};

///
/// Class that implements the edge iterator concept of a Multimodal::Graph
///
/// It is a wrapper around a VertexIterator and an OutEdgeIterator.
/// Basically, iterating over edges is done by a double for loop that iterates over each out edges of each vertex
///
class EdgeIterator :
    public boost::iterator_facade< EdgeIterator,
    Multimodal::Edge,
    boost::forward_traversal_tag,
/* reference */ Multimodal::Edge> {
public:
    EdgeIterator() : graph_( 0 ) {}
    EdgeIterator( const Multimodal::Graph& graph );

    void to_end();
    Multimodal::Edge dereference() const;
    void increment();
    bool equal( const EdgeIterator& v ) const;
protected:
    ///
    /// The underlying graph
    const Multimodal::Graph* graph_;

    ///
    /// A pair of VertexIterator
    Multimodal::VertexIterator vi_, vi_end_;

    ///
    /// A pair of OutEdgeIterator
    Multimodal::OutEdgeIterator ei_, ei_end_;

    friend std::ostream& operator<<( std::ostream& ostr, const EdgeIterator& it );
};

///
/// Class that implemented the property map vertex_index.
///
/// The goal is to map an integer in the range (0, num_vertices-1) to a vertex
class VertexIndexProperty {
public:
    typedef size_t                     value_type;
    typedef size_t&                    reference;
    typedef Tempus::Multimodal::Vertex key_type;
    typedef boost::vertex_property_tag category;

    VertexIndexProperty( const Graph& graph ) : graph_( graph ) {}
    size_t get_index( const Vertex& v ) const;

    size_t operator[] ( const Vertex& v ) const {
        return get_index( v );
    }
protected:
    const Multimodal::Graph& graph_;
};

//
// Boost graph functions declarations
// A Multimodal::Graph is an IncidenceGraph and a VertexAndEdgeListGraph

///
/// Number of vertices. Constant time
size_t num_vertices( const Graph& graph );
///
/// Number of edges. O(n) for n number of PT and POI edges
size_t num_edges( const Graph& graph );
///
/// Returns source vertex from an edge. Constant time (linear in number of PT networks)
Vertex source( const Edge& e, const Graph& graph );
///
/// Returns source vertex from an edge. Constant time (linear in number of PT networks)
Vertex target( const Edge& e, const Graph& graph );

///
/// Returns a range of VertexIterator. Constant time
std::pair<VertexIterator, VertexIterator> vertices( const Graph& graph );
///
/// Returns a range of EdgeIterator. Constant time
std::pair<EdgeIterator, EdgeIterator> edges( const Graph& graph );
///
/// Returns a range of EdgeIterator that allows to iterate on out edges of a vertex. Constant time
std::pair<OutEdgeIterator, OutEdgeIterator> out_edges( const Vertex& v, const Graph& graph );
///
/// Returns a range of EdgeIterator that allows to iterate on in edges of a vertex. Constant time
std::pair<InEdgeIterator, InEdgeIterator> in_edges( const Vertex& v, const Graph& graph );
///
/// Number of out edges for a vertex.
size_t out_degree( const Vertex& v, const Graph& graph );
///
/// Number of in edges for a vertex.
size_t in_degree( const Vertex& v, const Graph& graph );
///
/// Number of out and in edges for a vertex.
size_t degree( const Vertex& v, const Graph& graph );

///
/// Find an edge, based on a source and target vertex.
/// It does not implements AdjacencyMatrix, since it does not returns in constant time (linear in the number of edges)
std::pair< Edge, bool > edge( const Vertex& u, const Vertex& v, const Graph& graph );

///
/// Overloading of get()
VertexIndexProperty get( boost::vertex_index_t, const Multimodal::Graph& graph );

size_t get( const VertexIndexProperty& p, const Multimodal::Vertex& v );
}
}

namespace Tempus {

namespace Multimodal {
std::ostream& operator<<( std::ostream& out, const Vertex& v );
std::ostream& operator<<( std::ostream& out, const Edge& v );
}

///
/// Tests if a vertex exists. Works for Road::Vertex, PublicTransport::Vertex and Multimodal::Vertex
template <class G>
bool vertex_exists( typename boost::graph_traits<G>::vertex_descriptor v, G& graph )
{
    typename boost::graph_traits<G>::vertex_iterator vi, vi_end;

    for ( boost::tie( vi, vi_end ) = vertices( graph ); vi != vi_end; vi++ ) {
        if ( *vi == v ) {
            return true;
        }
    }

    return false;
}

///
/// Tests if an edge exists. Works for Road::Edge, PublicTransport::Edge and Multimodal::Edge
template <class G>
bool edge_exists( typename boost::graph_traits<G>::edge_descriptor v, G& graph )
{
    typename boost::graph_traits<G>::edge_iterator vi, vi_end;

    for ( boost::tie( vi, vi_end ) = edges( graph ); vi != vi_end; vi++ ) {
        if ( *vi == v ) {
            return true;
        }
    }

    return false;
}

///
/// Template magic used to abstract a graph object (either a vertex or an edge)
template <class G, class Tag>
struct vertex_or_edge {
    struct null_class {};
    typedef null_class property_type;
    typedef null_class descriptor;
};
template <class G>
struct vertex_or_edge<G, boost::vertex_property_tag> {
    typedef typename boost::graph_traits<G>::vertex_descriptor descriptor;
};
template <class G>
struct vertex_or_edge<G, boost::edge_property_tag> {
    typedef typename boost::graph_traits<G>::edge_descriptor descriptor;
};

}

#endif
