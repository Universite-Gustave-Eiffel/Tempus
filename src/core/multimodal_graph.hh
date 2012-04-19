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

namespace Tempus
{
    ///
    /// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
    struct MultimodalGraph
    {
	Road::Graph road;
	
	typedef std::map<db_id_t, PublicTransport::Network> NetworkMap;
	NetworkMap network_map;

	// network_id -> PublicTransport::Graph
	typedef std::map<db_id_t, PublicTransport::Graph> PublicTransportGraphList;
	PublicTransportGraphList public_transports;
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

    template <class G, class Tag>
    struct vertex_or_edge
    {
	typedef void property_type;
	typedef void descriptor;
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
    /// A FieldPropertyAccessor implementes a Readable Property Map concept and gives read access
    /// to the member of a vertex or edge
    /// For instance, FieldPropertyAccessor< Road::Edge, 
    template <class Graph, class Tag, class T, T vertex_or_edge<Graph, Tag>::property_type::*mptr>
    struct FieldPropertyAccessor
    {
	FieldPropertyAccessor( Graph& graph ) : graph_(graph) {}
	Graph& graph_;
    };
};

namespace boost
{
    template <class Graph, class Tag, class T, T Tempus::vertex_or_edge<Graph, Tag>::property_type::*mptr>
    T get( Tempus::FieldPropertyAccessor<Graph, Tag, T, mptr> pmap, typename Tempus::vertex_or_edge<Graph, Tag>::descriptor e )
    {
	return pmap.graph_[e].*mptr;
    }

    template <class Graph, class Tag, class T, T Tempus::vertex_or_edge<Graph, Tag>::property_type::*mptr>
    struct property_traits<Tempus::FieldPropertyAccessor<Graph, Tag, T, mptr> >
    {
	typedef T value_type;
	typedef T& reference;
	typedef typename Tempus::vertex_or_edge<Graph, Tag>::descriptor key_type;
	typedef Tag category;
    };
};

#endif
