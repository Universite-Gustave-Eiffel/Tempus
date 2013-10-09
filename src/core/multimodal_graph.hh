// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

#ifndef TEMPUS_MULTIMODAL_GRAPH_HH
#define TEMPUS_MULTIMODAL_GRAPH_HH

#include "common.hh"
#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "sub_map.hh"

namespace Db
{
    class Connection;
}

namespace Tempus
{
    // forward declarations
    namespace Multimodal
    {
	class VertexIterator;
	class OutEdgeIterator;
	class EdgeIterator;
    }
    // For debugging purposes
    std::ostream& operator<<( std::ostream& ostr, const Multimodal::VertexIterator& it );
    std::ostream& operator<<( std::ostream& ostr, const Multimodal::OutEdgeIterator& it );
    std::ostream& operator<<( std::ostream& ostr, const Multimodal::EdgeIterator& it );
}

namespace Tempus
{
    ///
    /// Multimodal namespace
    ///
    /// A Multimodal::Graph is a Road::Graph, a list of PublicTransport::Graph and a list of POIs
    ///
    namespace Multimodal
    {
	///
	/// A Multimodal::Vertex is either a Road::Vertex or PublicTransport::Vertex on a particular public transport network
	struct Vertex
	{
	    enum VertexType
	    {
		Road,            /// This vertex is a road vertex
		PublicTransport, /// This vertex is a public transport stop
		Poi              /// This vertex is a POI
	    };
	    VertexType type;

	    union
	    {
		const Road::Graph* road_graph;
		const PublicTransport::Graph* pt_graph;
		const POI* poi;
	    };

	    ///
	    /// The road vertex if this is relevant ( cannot be stored in a union since it has non trivial constructors )
	    Road::Vertex road_vertex;
	    ///
	    /// The public transport vertex if this is relevant ( cannot be stored in a union since it has non trivial constructors )
	    PublicTransport::Vertex pt_vertex;
	    
	    ///
	    /// Comparison operator
	    bool operator==( const Vertex& v ) const;
	    bool operator!=( const Vertex& v ) const;
	    bool operator<( const Vertex& v ) const;

	    Vertex() {}
	    Vertex( const Road::Graph* graph, Road::Vertex vertex );
	    Vertex( const PublicTransport::Graph* graph, PublicTransport::Vertex vertex );
	    Vertex( const POI* poi );
	};
	
	///
	/// A multimodal edge is a pair of multimodal vertices
	struct Edge
	{
	    ///
	    /// The source vertex
	    Multimodal::Vertex source;
	    ///
	    /// The target vertex
	    Multimodal::Vertex target;

	    enum ConnectionType
	    {
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
	    
	    Edge() {}
	    Edge( Multimodal::Vertex s, Multimodal::Vertex t ) : source(s), target(t) {}
	    
	    bool operator==( const Multimodal::Edge& e ) const
	    {
		return source == e.source && target == e.target;
	    }
	    bool operator!=( const Multimodal::Edge& e ) const
	    {
		return source != e.source || target != e.target;
	    }
	    bool operator<( const Multimodal::Edge& e ) const
	    {
		return source < e.source || ((source == e.source) && target < e.target);
	    }
	};

	///
	/// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
	struct Graph: boost::noncopyable
	{
            // declaration for boost::graph
            typedef Tempus::Multimodal::Vertex          vertex_descriptor;
            typedef Tempus::Multimodal::Edge            edge_descriptor;
            typedef Tempus::Multimodal::OutEdgeIterator out_edge_iterator;
            typedef Tempus::Multimodal::VertexIterator  vertex_iterator;
            typedef Tempus::Multimodal::EdgeIterator    edge_iterator;

            typedef boost::directed_tag                 directed_category;

            typedef boost::disallow_parallel_edge_tag   edge_parallel_category;
            typedef boost::incidence_graph_tag          traversal_category;

            typedef size_t vertices_size_type;
            typedef size_t edges_size_type;
            typedef size_t degree_size_type;

            // unused types (here to please boost::graph_traits<>)
            typedef void adjacency_iterator;
            typedef void in_edge_iterator;

            static inline vertex_descriptor null_vertex();

	    ///
	    /// The road graph
	    Road::Graph road;
	    
	    ///
	    /// Public transport networks
	    typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;
	    NetworkMap network_map;
	    
	    ///
	    /// Public transports graphs
	    /// network_id -> PublicTransport::Graph
	    /// This a sub_map that can thus be filtered to select only a subset
	    typedef sub_map<db_id_t, PublicTransport::Graph> PublicTransportGraphList;
	    PublicTransportGraphList public_transports;

	    ///
	    /// Point of interests
	    typedef std::map<db_id_t, POI> PoiList;
	    PoiList pois;
	    
	    ///
	    /// Variables used to store constants.
	    RoadTypes road_types;
	    TransportTypes transport_types;
	    
	    typedef std::map<std::string, Tempus::db_id_t> NameToId;
	    ///
	    /// Associative array that maps a road type name to a road type id
	    NameToId road_type_from_name;
	    ///
	    /// Associative array that maps a transport type name to a transport type id
	    NameToId transport_type_from_name;
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
	    VertexIterator() : graph_(0) {}
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

	    friend std::ostream& Tempus::operator<<( std::ostream& ostr, const Multimodal::VertexIterator& it );
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
                                           /* reference */ Multimodal::Edge>
	{
	public:
	    OutEdgeIterator() : graph_(0) {}
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

	    friend std::ostream& Tempus::operator<<( std::ostream& ostr, const OutEdgeIterator& it );
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
                                           /* reference */ Multimodal::Edge>
	{
	public:
	    EdgeIterator() : graph_(0) {}
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

	    friend std::ostream& Tempus::operator<<( std::ostream& ostr, const EdgeIterator& it );
	};

	///
	/// Class that implemented the property map vertex_index.
	///
	/// The goal is to map an integer in the range (0, num_vertices-1) to a vertex
	class VertexIndexProperty
	{
	public:
            typedef size_t                     value_type;
            typedef size_t&                    reference;
            typedef Tempus::Multimodal::Vertex key_type;
            typedef boost::vertex_property_tag category;

	    VertexIndexProperty( const Graph& graph ) : graph_(graph) {}
	    size_t get_index( const Vertex& v ) const;

	    size_t operator[] ( const Vertex& v ) const { return get_index( v ); }
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
	/// Number of edges. Constant time
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
	size_t out_degree( const Vertex& v, const Graph& graph );

	///
	/// Find an edge, based on a source and target vertex.
	/// It does not implements AdjacencyMatrix, since it does not returns in constant time (linear in the number of edges)
	std::pair< Edge, bool > edge( const Vertex& u, const Vertex& v, const Graph& graph );

	///
	/// Get the road edge if the given edge is a Road2Road
	/// else, return false
	std::pair< Road::Edge, bool > road_edge( const Multimodal::Edge& e );

	///
	/// Get the public transport edge if the given edge is a Transport2Transport
	/// else, return false
	std::pair< PublicTransport::Edge, bool > public_transport_edge( const Multimodal::Edge& e );

	///
	/// Overloading of get()
	VertexIndexProperty get( boost::vertex_index_t, const Multimodal::Graph& graph );

	size_t get( const VertexIndexProperty& p, const Multimodal::Vertex& v );
    }
}

namespace Tempus
{
    std::ostream& operator<<( std::ostream& out, const Multimodal::Vertex& v );
    std::ostream& operator<<( std::ostream& out, const Multimodal::Edge& v );

    ///
    /// Tests if a vertex exists. Works for Road::Vertex, PublicTransport::Vertex and Multimodal::Vertex
    template <class G>
    bool vertex_exists( typename boost::graph_traits<G>::vertex_descriptor v, G& graph )
    {
	typename boost::graph_traits<G>::vertex_iterator vi, vi_end;
	for ( boost::tie( vi, vi_end ) = vertices( graph ); vi != vi_end; vi++ )
	{
	    if ( *vi == v )
		return true;
	}
	return false;
    }

    ///
    /// Tests if an edge exists. Works for Road::Edge, PublicTransport::Edge and Multimodal::Edge
    template <class G>
    bool edge_exists( typename boost::graph_traits<G>::edge_descriptor v, G& graph )
    {
	typename boost::graph_traits<G>::edge_iterator vi, vi_end;
	for ( boost::tie( vi, vi_end ) = edges( graph ); vi != vi_end; vi++ )
	{
	    if ( *vi == v )
		return true;
	}
	return false;
    }

    ///
    /// Template magic used to abstract a graph object (either a vertex or an edge)
    template <class G, class Tag>
    struct vertex_or_edge
    {
	struct null_class {};
	typedef null_class property_type;
	typedef null_class descriptor;
    };
    template <class G>
    struct vertex_or_edge<G, boost::vertex_property_tag>
    {
	typedef typename boost::vertex_bundle_type<G>::type property_type;
	typedef typename boost::graph_traits<G>::vertex_descriptor descriptor;
    };
    template <class G>
    struct vertex_or_edge<G, boost::edge_property_tag>
    {
	typedef typename boost::edge_bundle_type<G>::type property_type;
	typedef typename boost::graph_traits<G>::edge_descriptor descriptor;
    };

}

#endif
