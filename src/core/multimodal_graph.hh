// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A MultimodalGraph is a Road::Graph and a list of PublicTransport::Graph
 */

#ifndef TEMPUS_MULTIMODAL_GRAPH_HH
#define TEMPUS_MULTIMODAL_GRAPH_HH

#include "common.hh"
#include "road_graph.hh"
#include "public_transport_graph.hh"

namespace Db
{
    class Connection;
};

namespace Tempus
{
    ///
    /// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
    struct MultimodalGraph
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
	typedef std::map<db_id_t, PublicTransport::Graph> PublicTransportGraphList;
	PublicTransportGraphList public_transports;

	///
	/// Point of interests
	typedef std::map<db_id_t, POI> PoiList;
	PoiList pois;

	///
	/// Variables used to store constants.
	/// For the sake of readability, always use them with their prefixing namespace
	RoadTypes road_types;
	TransportTypes transport_types;
	
	typedef std::map<std::string, Tempus::db_id_t> NameToId;
	NameToId road_type_from_name;
	NameToId transport_type_from_name;
    };

    template <class G>
    typename boost::graph_traits<G>::vertex_descriptor vertex_from_id( Tempus::db_id_t db_id, G& graph)
    {
    	typename boost::graph_traits<G>::vertex_iterator vi, vi_end;
    	for ( boost::tie( vi, vi_end ) = boost::vertices( graph ); vi != vi_end; vi++ )
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
    	for ( boost::tie( vi, vi_end ) = boost::edges( graph ); vi != vi_end; vi++ )
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
	for ( boost::tie( vi, vi_end ) = boost::vertices( graph ); vi != vi_end; vi++ )
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
	for ( boost::tie( vi, vi_end ) = boost::edges( graph ); vi != vi_end; vi++ )
	{
	    if ( *vi == v )
		return true;
	}
	return false;
    }

    ///
    /// Get 2D coordinates of a road or public transport vertex, from the database
    Point2D coordinates( Road::Vertex v, Db::Connection& db, Road::Graph& graph );
    Point2D coordinates( PublicTransport::Vertex v, Db::Connection& db, PublicTransport::Graph& graph );

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
//    template <class Graph, class Tag, class T, T vertex_or_edge<Graph, Tag>::property_type::*mptr>
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
