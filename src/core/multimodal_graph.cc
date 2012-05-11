#include <string>

#include <boost/format.hpp>

#include "multimodal_graph.hh"
#include "db.hh"

using namespace std;

namespace Tempus
{
    Point2D coordinates( const Road::Vertex& v, Db::Connection& db, const Road::Graph& graph )
    {
	string q = (boost::format( "SELECT x(geom), y(geom) FROM tempus.road_node WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<double>();
	p.y = res[0][1].as<double>();
	return p;
    }

    Point2D coordinates( const PublicTransport::Vertex& v, Db::Connection& db, const PublicTransport::Graph& graph )
    {
	string q = (boost::format( "SELECT x(geom), y(geom) FROM tempus.pt_stop WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<double>();
	p.y = res[0][1].as<double>();
	return p;
    }

    Point2D coordinates( const Multimodal::Vertex& v, Db::Connection& db, const Multimodal::Graph& graph )
    {
	if ( v.is_road )
	{
	    return coordinates( v.road.vertex, db, *v.road.graph );
	}
	// else
	return coordinates( v.pt.vertex, db, *v.pt.graph );
    }

    namespace Multimodal
    {
	bool Vertex::operator==( const Vertex& v ) const
	{
	    if ( is_road != v.is_road )
		return false;
	    if ( is_road )
	    {
		return road.vertex == v.road.vertex;
	    }
	    return ( pt.graph == v.pt.graph ) && ( pt.vertex == v.pt.vertex );
	}

	bool Vertex::operator!=( const Vertex& v ) const
	{
	    return !operator==( v );
	}
	
	bool Vertex::operator<( const Vertex& v ) const
	{
	    if ( is_road && !v.is_road )
		return true;
	    if ( !is_road && v.is_road )
		return false;
	    
	    if ( is_road ) // && v.is_road
	    {
		return road.vertex < v.road.vertex;
	    }
	    // else
	    if ( pt.graph != v.pt.graph )
		return pt.graph < v.pt.graph;
	    return pt.vertex < v.pt.vertex;
	}
	
	Vertex::Vertex( const Road::Graph* graph, Road::Vertex vertex )
	{
	    is_road = true;
	    road.graph = graph;
	    road.vertex = vertex;
	}
	Vertex::Vertex( const PublicTransport::Graph* graph, PublicTransport::Vertex vertex )
	{
	    is_road = false;
	    pt.graph = graph;
	    pt.vertex = vertex;
	}
    
	Edge::ConnectionType Edge::connection_type() const
	{
	    if ( source.is_road )
	    {
		if ( target.is_road )
		{
		    return Road2Road;
		}
		else
		{
		    return Road2Transport;
		}
	    }
	    else
	    {
		if ( target.is_road )
		{
		    return Transport2Road;
		}
		else
		{
		    return Transport2Transport;
		}
	    }
	}
	
	VertexIterator::VertexIterator( const Multimodal::Graph& graph )
	{
	    graph_ = &graph;
	    boost::tie( road_it_, road_it_end_ ) = boost::vertices( graph_->road );
	    pt_graph_it_ = graph_->public_transports.begin();
	    pt_graph_it_end_ = graph_->public_transports.end();
	    boost::tie( pt_it_, pt_it_end_ ) = boost::vertices( pt_graph_it_->second );
	}
	
	void VertexIterator::to_end()
	{
	    BOOST_ASSERT( graph_ != 0 );
	    road_it_ = road_it_end_;
	    pt_graph_it_ = pt_graph_it_end_;
	    pt_it_ = pt_it_end_;
	}
	
	Vertex& VertexIterator::dereference() const
	{
	    BOOST_ASSERT( graph_ != 0 );
	    vertex_.is_road = ( road_it_ != road_it_end_ );
	    if ( vertex_.is_road )
	    {
		vertex_.road.vertex = *road_it_;
		vertex_.road.graph = &graph_->road;
	    }
	    else
	    {
		vertex_.pt.graph = &(pt_graph_it_->second);
		vertex_.pt.vertex = *pt_it_;
	    }
	    return vertex_;
	}
	
	void VertexIterator::increment()
	{
	    BOOST_ASSERT( graph_ != 0 );
	    if ( road_it_ != road_it_end_ )
	    {
		road_it_++;
	    }
	    else
	    {
		if ( pt_it_ != pt_it_end_ )
		{
		    pt_it_++;
		}

		while ( pt_it_ == pt_it_end_ && pt_graph_it_ != pt_graph_it_end_ )
		{
		    pt_graph_it_++;
		    if ( pt_graph_it_ != pt_graph_it_end_ )
		    {
			// set on the beginning of the next pt_graph
			boost::tie( pt_it_, pt_it_end_ ) = boost::vertices( pt_graph_it_->second );
		    }
		}
	    }
	}

	bool VertexIterator::equal( const VertexIterator& v ) const
	{
	    // not the same road graph
	    if ( graph_ != v.graph_ )
		return false;

	    bool is_on_road = ( road_it_ != road_it_end_ );
	    if ( is_on_road )
	    {
		return road_it_ == v.road_it_;
	    }

	    // not on the same pt graph
	    if ( pt_graph_it_ != v.pt_graph_it_ )
		return false;
	    
	    // else, same pt graph
	    return pt_it_ == v.pt_it_;
	}
	
	EdgeIterator::EdgeIterator( const Multimodal::Graph& graph )
	{
	    graph_ = &graph;
	    boost::tie( vi_, vi_end_ ) = vertices( graph );
	    boost::tie( ei_, ei_end_ ) = out_edges( *vi_, *graph_ );
	}

	void EdgeIterator::to_end()
	{
	    BOOST_ASSERT( graph_ != 0 );
	    vi_ = vi_end_;
	    ei_ = ei_end_;
	    BOOST_ASSERT( vi_ == vi_end_ );
	    BOOST_ASSERT( ei_ == ei_end_ );
	}

	Multimodal::Edge& EdgeIterator::dereference() const
	{
	    BOOST_ASSERT( graph_ != 0 );
	    BOOST_ASSERT( ei_ != ei_end_ );
	    return *ei_;
	}

	void EdgeIterator::increment()
	{
	    BOOST_ASSERT( graph_ != 0 );
	    if ( ei_ != ei_end_ )
	    {
		ei_++;
	    }

	    while ( ei_ == ei_end_ && vi_ != vi_end_ )
	    {
		vi_++;
		if ( vi_ != vi_end_ )
		{
		    boost::tie( ei_, ei_end_ ) = out_edges( *vi_, *graph_ );
		}
	    }
	}
	
	bool EdgeIterator::equal( const EdgeIterator& v ) const
	{
	    // not the same road graph
	    if ( graph_ != v.graph_ )
		return false;

	    if ( vi_ != vi_end_ )
		return vi_ == v.vi_ && ei_ == v.ei_;
	    return v.vi_ == v.vi_end_;
	}

	OutEdgeIterator::OutEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source )
	{
	    graph_ = &graph;
	    source_ = source;
	    if ( source.is_road )
	    {
		boost::tie( road_it_, road_it_end_ ) = boost::out_edges( source.road.vertex, graph_->road );
	    }
	    else
	    {
		boost::tie( pt_it_, pt_it_end_ ) = boost::out_edges( source.pt.vertex, *source.pt.graph );
	    }
	    stop2road_connection_ = 0;
	    road2stop_connection_ = 0;
	}

	void OutEdgeIterator::to_end()
	{
	    pt_it_ = pt_it_end_;
	    road_it_ = road_it_end_;
	    stop2road_connection_ = 2;
	    road2stop_connection_ = -1;
	}

	Multimodal::Edge& OutEdgeIterator::dereference() const
	{
	    BOOST_ASSERT( graph_ != 0 );
	    Multimodal::Vertex mm_target;
	    edge_.source = source_;

	    if ( source_.is_road )
	    {
		BOOST_ASSERT( road_it_ != road_it_end_ );
		Road::Vertex r_target = boost::target( *road_it_, graph_->road );
		if ( road2stop_connection_ >= 0 && road2stop_connection_ < graph_->road[ *road_it_ ].stops.size() )
		{
		    size_t idx = road2stop_connection_;
		    const PublicTransport::Graph* pt_graph = graph_->road[ *road_it_ ].stops[ idx ]->graph;
		    PublicTransport::Vertex v = graph_->road[ *road_it_ ].stops[ idx ]->vertex;
		    mm_target = Multimodal::Vertex( pt_graph, v );
		}
		else
		{
		    mm_target = Multimodal::Vertex( &graph_->road, r_target );
		}
	    }
	    else
	    {
		const PublicTransport::Graph* pt_graph = source_.pt.graph;
		PublicTransport::Vertex r_source = source_.pt.vertex;
		if ( stop2road_connection_ == 0 )
		{
		    mm_target = Multimodal::Vertex( &graph_->road, boost::source( (*pt_graph)[ r_source ].road_section, graph_->road ) );
		}
		else if ( stop2road_connection_ == 1 )
		{
		    mm_target = Multimodal::Vertex( &graph_->road, boost::target( (*pt_graph)[ r_source ].road_section, graph_->road ) );
		}
		else
		{
		    PublicTransport::Vertex r_target = boost::target( *pt_it_, *pt_graph );
		    mm_target = Multimodal::Vertex( pt_graph, r_target );
		}
	    }
	    edge_.target = mm_target;
	    return edge_;
	}

	void OutEdgeIterator::increment()
	{
	    BOOST_ASSERT( graph_ != 0 );
	    if ( source_.is_road )
	    {
		if ( road2stop_connection_ < graph_->road[*road_it_].stops.size() )
		{
		    road2stop_connection_++;
		}
		else
		{
		    road2stop_connection_ = 0;
		    road_it_++;
		}
	    }
	    else
	    {
		if ( stop2road_connection_ < 2 )
		{
		    stop2road_connection_ ++;
		}
		else
		{
		    bool is_at_pt_end = ( pt_it_ == pt_it_end_ );
		    if ( !is_at_pt_end )
		    {
			pt_it_++;
		    }
		}
	    }
	}

	bool OutEdgeIterator::equal( const OutEdgeIterator& v ) const
	{
	    // not the same road graph
	    if ( graph_ != v.graph_ )
		return false;

	    if ( source_ != v.source_ )
		return false;

	    if ( source_.is_road )
	    {
		if ( road_it_ == road_it_end_ )
		    return v.road_it_ == v.road_it_end_;

		return road_it_ == v.road_it_ && road2stop_connection_ == v.road2stop_connection_;
	    }

	    // else
	    if ( stop2road_connection_ != v.stop2road_connection_ )
		return false;

	    return pt_it_ == v.pt_it_;
	}

	VertexIndexProperty get( boost::vertex_index_t, const Multimodal::Graph& graph ) { return VertexIndexProperty( graph ); }
	EdgeIndexProperty get( boost::edge_index_t, const Multimodal::Graph& graph ) { return EdgeIndexProperty( graph ); }

	size_t get( const VertexIndexProperty& p, const Multimodal::Vertex& v ) { return p.get_index( v ); }
	size_t get( const EdgeIndexProperty& p, const Multimodal::Edge& e ) { return p.get_index( e ); }

	// FIXME : slow ?
	size_t VertexIndexProperty::get_index( const Vertex& v ) const
	{
	    if ( v.is_road )
	    {
		return boost::get( boost::get( boost::vertex_index, *v.road.graph ), v.road.vertex );
	    }
	    // else
	    size_t n = num_vertices( graph_.road );
	    Multimodal::Graph::PublicTransportGraphList::const_iterator it;
	    for ( it = graph_.public_transports.begin(); it != graph_.public_transports.end(); it++ )	    
	    {
		if ( &it->second != v.pt.graph )
		{
		    n += num_vertices( it->second );
		}
		else
		{
		    n += boost::get( boost::get( boost::vertex_index, *v.pt.graph ), v.pt.vertex );
		    break;
		}
	    }
	    return n;
	}

	size_t num_vertices( const Graph& graph )
	{
	    size_t n = 0;
	    Multimodal::Graph::PublicTransportGraphList::const_iterator it;
	    for ( it = graph.public_transports.begin(); it != graph.public_transports.end(); it++ )
	    {
		n += num_vertices( it->second );
	    }
	    return n + num_vertices( graph.road );
	}
	size_t num_edges( const Graph& graph )
	{
	    size_t n = 0;
	    Multimodal::Graph::PublicTransportGraphList::const_iterator it;
	    for ( it = graph.public_transports.begin(); it != graph.public_transports.end(); it++ )
	    {
		n += num_edges( it->second );
		n += num_vertices( it->second ) * 4;
	    }
	    return n + num_edges( graph.road ) * 2;
	}
	Vertex& source( Edge& e, const Graph& graph )
	{
	    return e.source;
	}
	Vertex& target( Edge& e, const Graph& graph )
	{
	    return e.target;
	}
	pair<VertexIterator, VertexIterator> vertices( const Graph& graph )
	{
	    VertexIterator v_begin( graph ), v_end( graph );
	    v_end.to_end();
	    return std::make_pair( v_begin, v_end );
	}
	pair<EdgeIterator, EdgeIterator> edges( const Graph& graph )
	{
	    EdgeIterator v_begin( graph ), v_end( graph );
	    v_end.to_end();
	    return std::make_pair( v_begin, v_end );
	}
	
	pair<OutEdgeIterator, OutEdgeIterator> out_edges( const Vertex& v, const Graph& graph )
	{
	    OutEdgeIterator v_begin( graph, v ), v_end( graph, v );
	    v_end.to_end();
	    return std::make_pair( v_begin, v_end );
	}

	size_t out_degree( Vertex& v, const Graph& graph )
	{
	    if ( v.is_road )
	    {
		size_t sum = 0;
		Road::OutEdgeIterator ei, ei_end;
		for ( boost::tie(ei, ei_end) = out_edges( v.road.vertex, graph.road ); ei != ei_end; ei++ )
		{
		    sum += graph.road[ *ei ].stops.size() + 1;
		}
		return sum;
	    }
	    // else
	    return out_degree( v.pt.vertex, *v.pt.graph ) + 2;
	}

	std::pair< Edge, bool> edge( const Vertex& u, const Vertex& v, const Graph& graph )
	{
	    Multimodal::Edge e;
	    Multimodal::EdgeIterator ei, ei_end;
	    bool found = false;
	    for ( boost::tie( ei, ei_end ) = edges( graph ); ei != ei_end; ei++ )
	    {
		if ( source( *ei, graph ) == u && target( *ei, graph ) == v )
		{
		    found = true;
		    e = *ei;
		}
	    }
	    return std::make_pair( e, found );
	}
    };
	
    ostream& operator<<( ostream& out, const Multimodal::Vertex& v )
    {
	if ( v.is_road )
	{
	    out << "R" << (*v.road.graph)[v.road.vertex].db_id;
	}
	else
	{
	    out << "PT" << (*v.pt.graph)[v.pt.vertex].db_id;
	}
	return out;
    }

    ostream& operator<<( ostream& out, const Multimodal::Edge& e )
    {
	switch (e.connection_type())
	{
	case Multimodal::Edge::Road2Road:
	    out << "Road2Road ";
	    break;
	case Multimodal::Edge::Road2Transport:
	    out << "Road2Transport ";
	    break;
	case Multimodal::Edge::Transport2Road:
	    out << "Transport2Road ";
	    break;
	case Multimodal::Edge::Transport2Transport:
	    out << "Transport2Transport ";
	    break;
	}
	out << "(" << e.source << "," << e.target << ")";
	return out;
    }
};
