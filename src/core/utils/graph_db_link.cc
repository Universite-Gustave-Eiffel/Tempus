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

#include <string>
#include <boost/format.hpp>

#include "graph_db_link.hh"

using namespace std;

namespace Tempus {
Point2D coordinates( const Road::Vertex& v, Db::Connection& db, const Road::Graph& graph )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.road_node WHERE id=%1%" ) % graph[v].db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<double>() );
    p.set_y( res[0][1].as<double>() );
    return p;
}

Point2D coordinates( const PublicTransport::Vertex& v, Db::Connection& db, const PublicTransport::Graph& graph )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.pt_stop WHERE id=%1%" ) % graph[v].db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<double>() );
    p.set_y( res[0][1].as<double>() );
    return p;
}

Point2D coordinates( const POI* poi, Db::Connection& db )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.poi WHERE id=%1%" ) % poi->db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<double>() );
    p.set_y( res[0][1].as<double>() );
    return p;
}

Point2D coordinates( const Multimodal::Vertex& v, Db::Connection& db, const Multimodal::Graph& )
{
    if ( v.type() == Multimodal::Vertex::Road ) {
        return coordinates( v.road_vertex(), db, *v.road_graph() );
    }
    else if ( v.type() == Multimodal::Vertex::PublicTransport ) {
    return coordinates( v.pt_vertex(), db, *v.pt_graph() );
    }

    // else
return coordinates( v.poi(), db );
}

}
