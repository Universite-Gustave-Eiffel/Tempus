// (c) 2013 Oslandia
// MIT License

#include <string>
#include <boost/format.hpp>

#include "graph_db_link.hh"

using namespace std;

namespace Tempus
{
    Point2D coordinates( const Road::Vertex& v, Db::Connection& db, const Road::Graph& graph )
    {
	string q = (boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.road_node WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<double>();
	p.y = res[0][1].as<double>();
	return p;
    }

    Point2D coordinates( const PublicTransport::Vertex& v, Db::Connection& db, const PublicTransport::Graph& graph )
    {
	string q = (boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.pt_stop WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<double>();
	p.y = res[0][1].as<double>();
	return p;
    }

    Point2D coordinates( const POI* poi, Db::Connection& db )
    {
	string q = (boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.poi WHERE id=%1%" ) % poi->db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<double>();
	p.y = res[0][1].as<double>();
	return p;
    }

    Point2D coordinates( const Multimodal::Vertex& v, Db::Connection& db, const Multimodal::Graph& )
    {
	if ( v.type == Multimodal::Vertex::Road )
	{
	    return coordinates( v.road_vertex, db, *v.road_graph );
	}
	else if ( v.type == Multimodal::Vertex::PublicTransport )
	{
	    return coordinates( v.pt_vertex, db, *v.pt_graph );
	}
	// else
	return coordinates( v.poi, db );
    }

}
