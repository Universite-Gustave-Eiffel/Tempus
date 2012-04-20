#include <string>

#include <boost/format.hpp>

#include "multimodal_graph.hh"
#include "db.hh"

using namespace std;

namespace Tempus
{
    Point2D coordinates( Road::Vertex v, Db::Connection& db, Road::Graph& graph )
    {
	string q = (boost::format( "SELECT x(geom), y(geom) FROM tempus.road_node WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<Tempus::db_id_t>();
	p.y = res[0][1].as<Tempus::db_id_t>();
	return p;
    }

    Point2D coordinates( PublicTransport::Vertex v, Db::Connection& db, PublicTransport::Graph& graph )
    {
	string q = (boost::format( "SELECT x(geom), y(geom) FROM tempus.pt_stop WHERE id=%1%" ) % graph[v].db_id).str();
	Db::Result res = db.exec( q );
	BOOST_ASSERT( res.size() > 0 );
	Point2D p;
	p.x = res[0][0].as<Tempus::db_id_t>();
	p.y = res[0][1].as<Tempus::db_id_t>();
	return p;
    }
};
