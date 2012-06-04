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
};

namespace Tempus
{
    ///
    /// Multimodal namespace
    ///
    /// A Multimodal::Graph is a Road::Graph and a list of PublicTransport::Graph
    ///
    namespace Multimodal
    {
	///
	/// A Multimodal::Vertex is either a Road::Vertex or PublicTransport::Vertex on a particular public transport network
	struct Vertex
	{
	    enum VertexType
	    {
		Road,
		PublicTransport,
		Poi
	    };
	    VertexType type;

	    union
	    {
		const Road::Graph* road_graph;
		const PublicTransport::Graph* pt_graph;
		const POI* poi;
	    };
	    // things that cannot be stored in the union ( have non trivial constructors )
	    // If it's a road
	    Road::Vertex road_vertex;
	    // If it's a public transport
	    PublicTransport::Vertex pt_vertex;
	    
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
	    Multimodal::Vertex source;
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
		return source < e.source && target < e.target;
	    }
	};

	///
	/// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
	struct Graph
	{
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
	    NameToId road_type_from_name;
	    NameToId transport_type_from_name;
	};

	struct VertexIterator :
	    public boost::iterator_facade< VertexIterator,
					   Vertex,
					   boost::forward_traversal_tag >
	
	{
	public:
	    VertexIterator() : graph_(0) {}
	    VertexIterator( const Graph& graph );

	    void to_end();
	    Vertex& dereference() const;
	    void increment();
	    bool equal( const VertexIterator& v ) const;
	protected:
	    Road::VertexIterator road_it_, road_it_end_;
	    Multimodal::Graph::PublicTransportGraphList::const_subset_iterator pt_graph_it_, pt_graph_it_end_;
	    Multimodal::Graph::PoiList::const_iterator poi_it_, poi_it_end_;
	    PublicTransport::VertexIterator pt_it_, pt_it_end_;
	    const Multimodal::Graph* graph_;
	    mutable Multimodal::Vertex vertex_;
	};

	struct OutEdgeIterator :
	    public boost::iterator_facade< OutEdgeIterator,
					   Multimodal::Edge,
					   boost::forward_traversal_tag >
	{
	protected:
	    Multimodal::Vertex source_;
	    const Multimodal::Graph* graph_;
	    mutable Multimodal::Edge edge_;
	    Road::OutEdgeIterator road_it_, road_it_end_;
	    PublicTransport::OutEdgeIterator pt_it_, pt_it_end_;
	    // 0 / 1 / 2 stop2road connection
	    size_t stop2road_connection_;
	    // 0 .. N, -1 road2stop connection
	    int road2stop_connection_;

	    // 0 -> N
	    int road2poi_connection_;
	    // O -> 2
	    int poi2road_connection_;
	public:
	    OutEdgeIterator() : graph_(0) {}
	    OutEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source );

	    void to_end();
	    Multimodal::Edge& dereference() const;
	    void increment();
	    bool equal( const OutEdgeIterator& v ) const;
	};

	struct EdgeIterator :
	    public boost::iterator_facade< EdgeIterator,
					   Multimodal::Edge,
					   boost::forward_traversal_tag >
	{
	protected:
	    const Multimodal::Graph* graph_;
	    Multimodal::VertexIterator vi_, vi_end_;
	    Multimodal::OutEdgeIterator ei_, ei_end_;
	public:
	    EdgeIterator() : graph_(0) {}
	    EdgeIterator( const Multimodal::Graph& graph );

	    void to_end();
	    Multimodal::Edge& dereference() const;
	    void increment();
	    bool equal( const EdgeIterator& v ) const;
	};

	class VertexIndexProperty
	{
	public:
	    VertexIndexProperty( const Graph& graph ) : graph_(graph) {}
	    size_t get_index( const Vertex& v ) const;

	    size_t operator[] ( const Vertex& v ) const { return get_index( v ); }
	protected:
	    const Multimodal::Graph& graph_;
	};

	class EdgeIndexProperty
	{
	public:
	    EdgeIndexProperty( const Graph& graph ) : graph_(graph) {}
	    size_t get_index( const Edge& v ) const {/* TODO */ return 0; }

	    size_t operator[] ( const Edge& e ) const { return get_index( e ); }
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
	Vertex& source( Edge& e, const Graph& graph );
	///
	/// Returns source vertex from an edge. Constant time (linear in number of PT networks)
	Vertex& target( Edge& e, const Graph& graph );

	///
	/// Returns a range of VertexIterator. Constant time
	std::pair<VertexIterator, VertexIterator> vertices( const Graph& graph );
	///
	/// Returns a range of EdgeIterator. Constant time
	std::pair<EdgeIterator, EdgeIterator> edges( const Graph& graph );
	///
	/// Returns a range of EdgeIterator that allows to iterate on out edges of a vertex. Constant time
	std::pair<OutEdgeIterator, OutEdgeIterator> out_edges( const Vertex& v, const Graph& graph );
	size_t out_degree( Vertex& v, const Graph& graph );

	///
	/// Find an edge, based on a source and target vertex.
	/// It does not implements AdjacencyMatrix, since it does not returns in constant time (linear in the number of edges)
	std::pair< Edge, bool > edge( const Vertex& u, const Vertex& v, const Graph& graph );

	///
	/// Overloading of get()
	VertexIndexProperty get( boost::vertex_index_t, const Multimodal::Graph& graph );
	EdgeIndexProperty get( boost::edge_index_t, const Multimodal::Graph& graph );

	size_t get( const VertexIndexProperty& p, const Multimodal::Vertex& v );
	size_t get( const EdgeIndexProperty& p, const Multimodal::Edge& e );
    };
};

namespace boost
{

    template <>
    struct property_traits<Tempus::Multimodal::VertexIndexProperty>
    {
	typedef size_t value_type;
	typedef size_t& reference;
	typedef Tempus::Multimodal::Vertex key_type;
	typedef boost::vertex_property_tag category;
    };

    template <>
    struct property_traits<Tempus::Multimodal::EdgeIndexProperty>
    {
	typedef size_t value_type;
	typedef size_t& reference;
	typedef Tempus::Multimodal::Edge key_type;
	typedef boost::edge_property_tag category;
    };

    //
    // Boost graph traits definition
    template <>
    struct graph_traits< Tempus::Multimodal::Graph >
    {
	typedef Tempus::Multimodal::Vertex vertex_descriptor;
	typedef Tempus::Multimodal::Edge edge_descriptor;
        typedef Tempus::Multimodal::OutEdgeIterator out_edge_iterator;
        typedef Tempus::Multimodal::VertexIterator vertex_iterator;
        typedef Tempus::Multimodal::EdgeIterator edge_iterator;

	typedef directed_tag directed_category;

	typedef disallow_parallel_edge_tag edge_parallel_category;
        typedef incidence_graph_tag traversal_category;

        typedef size_t vertices_size_type;
        typedef size_t edges_size_type;
        typedef size_t degree_size_type;

        static inline vertex_descriptor null_vertex();
    };
};

namespace Tempus
{
    std::ostream& operator<<( std::ostream& out, const Multimodal::Vertex& v );
    std::ostream& operator<<( std::ostream& out, const Multimodal::Edge& v );

    template <class G>
    typename boost::graph_traits<G>::vertex_descriptor vertex_from_id( Tempus::db_id_t db_id, G& graph)
    {
    	typename boost::graph_traits<G>::vertex_iterator vi, vi_end;
    	for ( boost::tie( vi, vi_end ) = vertices( graph ); vi != vi_end; vi++ )
    	{
    	    if ( graph[*vi].db_id == db_id )
    		return *vi;
    	}
    	// null element
    	return typename boost::graph_traits<G>::vertex_descriptor();
    }

    template <class G>
    typename boost::graph_traits<G>::edge_descriptor edge_from_id( Tempus::db_id_t db_id, G& graph)
    {
    	typename boost::graph_traits<G>::edge_iterator vi, vi_end;
    	for ( boost::tie( vi, vi_end ) = edges( graph ); vi != vi_end; vi++ )
    	{
    	    if ( graph[*vi].db_id == db_id )
    		return *vi;
    	}
    	// null element
    	return typename boost::graph_traits<G>::edge_descriptor();
    }

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

    ///
    /// A FieldPropertyAccessor implements a Readable Property Map concept and gives read access
    /// to the member of a vertex or edge
    template <class Graph, class Tag, class T, class Member>
    struct FieldPropertyAccessor
    {
	FieldPropertyAccessor( Graph& graph, Member mem ) : graph_(graph), mem_(mem) {}
	Graph& graph_;
	Member mem_;
    };

    ///
    /// A FunctionPropertyAccessor implements a Readable Property Map concept
    /// by means of a function application on a vertex or edge of a graph
    template <class Graph, class Tag, class T, class Function>
    struct FunctionPropertyAccessor
    {
	FunctionPropertyAccessor( Graph& graph, Function fct ) : graph_(graph), fct_(fct) {}
	Graph& graph_;
	Function fct_;
    };
};

namespace boost
{
    template <class Graph, class Tag, class T, class Member>
    T get( Tempus::FieldPropertyAccessor<Graph, Tag, T, Member> pmap, typename Tempus::vertex_or_edge<Graph, Tag>::descriptor e )
    {
	return pmap.graph_[e].*(pmap.mem_);
    }

    template <class Graph, class Tag, class T, class Member>
    struct property_traits<Tempus::FieldPropertyAccessor<Graph, Tag, T, Member> >
    {
	typedef T value_type;
	typedef T& reference;
	typedef typename Tempus::vertex_or_edge<Graph, Tag>::descriptor key_type;
	typedef Tag category;
    };


    template <class Graph, class Tag, class T, class Function>
    T get( Tempus::FunctionPropertyAccessor<Graph, Tag, T, Function> pmap, typename Tempus::vertex_or_edge<Graph, Tag>::descriptor e )
    {
	return pmap.fct_( pmap.graph_, e );
    }

    template <class Graph, class Tag, class T, class Function>
    struct property_traits<Tempus::FunctionPropertyAccessor<Graph, Tag, T, Function> >
    {
	typedef T value_type;
	typedef T& reference;
	typedef typename Tempus::vertex_or_edge<Graph, Tag>::descriptor key_type;
	typedef Tag category;
    };
};

#endif
