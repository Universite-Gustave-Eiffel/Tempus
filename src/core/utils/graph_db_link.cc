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

std::string geometry_wkb( const Multimodal::Edge& e, Db::Connection& db )
{
    std::string res_wkb;
    std::string query;
    if ( e.connection_type() == Multimodal::Edge::Road2Road ) {
        db_id_t id = (*e.source().road_graph())[e.road_edge()].db_id();
        query = ( boost::format( "SELECT st_asbinary(geom) from "
                                 "tempus.road_section where id=%1%"
                                 ) % id ).str();
    }
    else if ( e.connection_type() == Multimodal::Edge::Road2Transport ) {
        db_id_t o_id = (*e.source().road_graph())[e.source().road_vertex()].db_id();
        db_id_t d_id = (*e.target().pt_graph())[e.target().pt_vertex()].db_id();

        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
                                             "(select geom from tempus.road_node where id=%1%) as t1, "
                                             "(select geom from tempus.pt_stop where id=%2%) as t2 "
                                             ) % o_id % d_id ).str();
        
    }
    else if ( e.connection_type() == Multimodal::Edge::Transport2Road ) {
        db_id_t o_id = (*e.source().pt_graph())[e.source().pt_vertex()].db_id();
        db_id_t d_id = (*e.target().road_graph())[e.target().road_vertex()].db_id();

        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
                                             "(select geom from tempus.pt_stop where id=%1%) as t1, "
                                             "(select geom from tempus.road_node where id=%2%) as t2 "
                                             ) % o_id % d_id ).str();
        
    }
    else if ( e.connection_type() == Multimodal::Edge::Road2Poi ) {
        db_id_t o_id = (*e.source().road_graph())[e.source().road_vertex()].db_id();
        db_id_t d_id = e.target().poi()->db_id();

        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
                                             "(select geom from tempus.road_node where id=%1%) as t1, "
                                             "(select geom from tempus.poi where id=%2%) as t2 "
                                             ) % o_id % d_id ).str();
        
    }
    else if ( e.connection_type() == Multimodal::Edge::Poi2Road ) {
        db_id_t o_id = e.source().poi()->db_id();
        db_id_t d_id = (*e.target().road_graph())[e.target().road_vertex()].db_id();

        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
                                             "(select geom from tempus.poi where id=%1%) as t1, "
                                             "(select geom from tempus.road_node where id=%2%) as t2 "
                                             ) % o_id % d_id ).str();
        
    }
    else if ( e.connection_type() == Multimodal::Edge::Transport2Transport ) {
        db_id_t o_id = (*e.source().pt_graph())[e.source().pt_vertex()].db_id();
        db_id_t d_id = (*e.target().pt_graph())[e.target().pt_vertex()].db_id();

        query = ( boost::format( "SELECT st_asbinary(geom) from "
                                 "tempus.pt_section where stop_from=%1% and stop_to=%2%"
                                 ) % o_id % d_id ).str();
    }

    Db::Result res = db.exec( query );
    if ( ! res.size() ) {
        throw std::runtime_error("No result for " + query);
    }
    std::string wkb = res[0][0].as<std::string>();
    // get rid of the heading '\x'
    res_wkb = wkb.substr( 2 );
    return res_wkb;
}

}
