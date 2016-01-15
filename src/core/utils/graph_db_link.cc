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
#include "../multimodal_graph.hh"

using namespace std;

namespace Tempus {
Point2D coordinates( const Road::Vertex& v, Db::Connection& db, const Road::Graph& graph )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.road_node WHERE id=%1%" ) % graph[v].db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<float>() );
    p.set_y( res[0][1].as<float>() );
    return p;
}

Point2D coordinates( const PublicTransport::Vertex& v, Db::Connection& db, const PublicTransport::Graph& graph )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.pt_stop WHERE id=%1%" ) % graph[v].db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<float>() );
    p.set_y( res[0][1].as<float>() );
    return p;
}

Point2D coordinates( const POI* poi, Db::Connection& db )
{
    string q = ( boost::format( "SELECT st_x(geom), st_y(geom) FROM tempus.poi WHERE id=%1%" ) % poi->db_id() ).str();
    Db::Result res = db.exec( q );
    BOOST_ASSERT( res.size() > 0 );
    Point2D p;
    p.set_x( res[0][0].as<float>() );
    p.set_y( res[0][1].as<float>() );
    return p;
}

Point2D coordinates( const Multimodal::Vertex& v, Db::Connection& db, const Multimodal::Graph& )
{
    if ( v.type() == Multimodal::Vertex::Road ) {
        return coordinates( v.road_vertex(), db, v.graph()->road() );
    }
    else if ( v.type() == Multimodal::Vertex::PublicTransport ) {
    return coordinates( v.pt_vertex(), db, *v.pt_graph() );
    }

    // else
return coordinates( v.poi(), db );
}

void get_edge_info_from_db( const MMEdge& e, Db::Connection& db, std::string& wkb, std::string& initial_name, std::string& final_name )
{
    std::string res_wkb;
    std::string query;
    if ( e.source().type() == MMVertex::Road && e.target().type() == MMVertex::Road ) {
        query = ( boost::format( "select st_asbinary(geom), road_name, road_name from "
                                 "(select geom, road_name from tempus.road_section where node_from=%1% and node_to=%2% "
                                 "union "
                                 "select geom, road_name from tempus.road_section where node_to=%1% and node_from=%2%) t "
                                 "where geom is not null limit 1"
                                 ) % e.source().id() % e.target().id() ).str();
    }
    else if ( e.source().type() == MMVertex::Road && e.target().type() == MMVertex::Transport ) {
        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)), "
                                 "(select road_name from tempus.road_section where id=t2.road_section_id), "
                                 "(select name from tempus.pt_stop where id=%2%) from "
                                 "  (select geom from tempus.road_node where id=%1%) as t1, "
                                 "  (select geom, road_section_id from tempus.pt_stop where id=%2%) as t2 "
                                 ) % e.source().id() % e.target().id() ).str();
        
    }
    else if ( e.source().type() == MMVertex::Transport && e.target().type() == MMVertex::Road ) {
        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)), "
                                 "(select name from tempus.pt_stop where id=%1%), "
                                 "(select road_name from tempus.road_section where id=t1.road_section_id) from "
                                 "  (select geom, road_section_id from tempus.pt_stop where id=%1%) as t1, "
                                 "  (select geom from tempus.road_node where id=%2%) as t2 "
                                 ) % e.source().id() % e.target().id() ).str();
        
    }
    else if ( e.source().type() == MMVertex::Road && e.target().type() == MMVertex::Poi ) {
        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)), "
                                 "(select road_name from tempus.road_section where id=t2.road_section_id), "
                                 "(select name from tempus.poi where id=%2%) from "
                                 "(select geom from tempus.road_node where id=%1%) as t1, "
                                 "(select geom, road_section_id from tempus.poi where id=%2%) as t2 "
                                 ) % e.source().id() % e.target().id() ).str();
        
    }
    else if ( e.source().type() == MMVertex::Poi && e.target().type() == MMVertex::Road ) {
        query = ( boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)), "
                                 "(select name from tempus.poi where id=%1%), "
                                 "(select road_name from tempus.road_section where id=t1.road_section_id) from "
                                 "(select geom, road_section_id from tempus.poi where id=%1%) as t1, "
                                 "(select geom from tempus.road_node where id=%2%) as t2 "
                                 ) % e.source().id() % e.target().id() ).str();
        
    }
    else if ( e.source().type() == MMVertex::Transport && e.target().type() == MMVertex::Transport ) {
        query = ( boost::format( "SELECT st_asbinary(geom), (select name from tempus.pt_stop where id=%1%), (select name from tempus.pt_stop where id=%2%) from "
                                 "tempus.pt_section where stop_from=%1% and stop_to=%2%"
                                 ) % e.source().id() % e.target().id() ).str();
    }

    Db::Result res = db.exec( query );
    wkb = res[0][0].as<std::string>();
    // get rid of the heading '\x'
    if ( wkb.size() > 2 ) {
        wkb = wkb.substr( 2 );
    }
    initial_name = res[0][1].as<std::string>();
    final_name = res[0][2].as<std::string>();
}

void fill_roadmap_from_db( Roadmap::StepIterator itbegin, Roadmap::StepIterator itend, Db::Connection& db  )
{
    // road node id -> road step*
    std::map<db_id_t, Roadmap::RoadStep*> road_steps;
    // pt stop id -> pt step*
    std::map<db_id_t, std::map<db_id_t, Roadmap::PublicTransportStep*>> pt_steps;
    // pt trip id -> vector of pt step*
    std::map<db_id_t, std::vector<Roadmap::PublicTransportStep*>> pt_route;
    for ( Roadmap::StepIterator it = itbegin; it != itend; it++ ) {
        if ( it->step_type() == Roadmap::Step::RoadStep ) {
            Roadmap::RoadStep* road_step = static_cast<Roadmap::RoadStep*>( &*it );
            road_steps[ road_step->road_edge_id() ] = road_step;
        }
        else if ( it->step_type() == Roadmap::Step::PublicTransportStep ) {
            Roadmap::PublicTransportStep* pt_step = static_cast<Roadmap::PublicTransportStep*>( &*it );
            db_id_t from_id = pt_step->departure_stop();
            db_id_t to_id = pt_step->arrival_stop();
            pt_steps[from_id][to_id] = pt_step;
            pt_route[pt_step->trip_id()].push_back( pt_step );
        }
        else if ( it->step_type() == Roadmap::Step::TransferStep ) {
            auto tr_step = static_cast<Roadmap::TransferStep*>( &*it );
            std::string wkb;
            std::string name1, name2;
            // WARNING: transfer steps are here requested one by one
            get_edge_info_from_db( *tr_step, db, wkb, name1, name2 );
            tr_step->set_initial_name( name1 );
            tr_step->set_final_name( name2 );
            tr_step->set_geometry_wkb( wkb );
        }
    }

    //
    // road steps
    if ( !road_steps.empty() ) {
        std::string q = "SELECT id, road_name, ST_AsBinary(geom) FROM tempus.road_section WHERE id IN(";
        for ( auto it = road_steps.begin(); it != road_steps.end(); it++ ) {
            if ( it != road_steps.begin() ) {
                q += ",";
            }
            q += to_string( it->first );
        }
        q += ")";

        Db::Result res = db.exec( q );
        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t id = res[i][0];
            std::string road_name = res[i][1];
            std::string wkb = res[i][2];
            // get rid of the heading '\x'
            if ( wkb.size() > 2 ) {
                wkb = wkb.substr( 2 );
            }
            road_steps[id]->set_road_name( road_name );
            road_steps[id]->set_geometry_wkb( wkb );
        }
    }

    //
    // PT steps geometry and stop names
    if ( !pt_steps.empty() ) {
        std::string q = "SELECT stop_from, stop_to, (select name from tempus.pt_stop where id=stop_from), (select name from tempus.pt_stop where id=stop_to), ST_AsBinary(geom) "
            "FROM tempus.pt_section WHERE ARRAY[stop_from,stop_to] IN (";
        size_t l = 0;
        for ( auto p : pt_steps ) {
            for ( auto p2 : p.second ) {
                if ( l ) {
                    q += ",";
                }
                q += "ARRAY[" + to_string( p.first ) + "," + to_string( p2.first ) + "]";
                l++;
            }
        }
        q += ")";

        Db::Result res = db.exec( q );
        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t from = res[i][0];
            db_id_t to = res[i][1];
            std::string from_name = res[i][2];
            std::string to_name = res[i][3];
            std::string wkb = res[i][4];
            // get rid of the heading '\x'
            if ( wkb.size() > 2 ) {
                wkb = wkb.substr( 2 );
            }
            pt_steps[from][to]->set_departure_name( from_name );
            pt_steps[from][to]->set_arrival_name( to_name );
            pt_steps[from][to]->set_geometry_wkb( wkb );
        }
    }

    // PT route info
    if ( !pt_route.empty() ) {
        std::string q = "SELECT t.id, r.short_name, r.long_name, r.transport_mode FROM tempus.pt_trip as t, tempus.pt_route as r WHERE t.route_id = r.id AND t.id IN (";
        size_t l = 0;
        for ( auto p : pt_route ) {
            if ( l ) {
                q += ",";
            }
            q += to_string( p.first ); // trip id
            l++;
        }
        q += ")";

        Db::Result res = db.exec( q );
        for ( size_t i = 0; i < res.size(); i++ ) {
            db_id_t trip_id = res[i][0];
            std::string short_name = res[i][1];
            std::string long_name = res[i][2];
            db_id_t transport_mode = res[i][3];

            if ( pt_route.find( trip_id ) == pt_route.end() ) {
                continue;
            }

            for ( auto step : pt_route[trip_id] ) {
                step->set_route( short_name + " - " + long_name );
                step->set_transport_mode( transport_mode );
            }
        }
    }
}

}
