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
#include <boost/variant/get.hpp>

#include "multimodal_graph.hh"
#include "db.hh"

using namespace std;

namespace Tempus {
namespace Multimodal {


Vertex::Vertex() : is_null_(true)
{
}

Vertex::Vertex( const Road::Graph* graph, Road::Vertex vertex ) : is_null_(false), union_( RoadVertex_(graph,vertex) )
{
}

Vertex::Vertex( const PublicTransport::Graph* graph, PublicTransport::Vertex vertex ) : is_null_(false), union_( PtVertex_(graph, vertex) )
{
}
Vertex::Vertex( const POI* ppoi ) : is_null_(false), union_( ppoi )
{
}

Vertex::VertexType Vertex::type() const
{
    return static_cast<Vertex::VertexType>(union_.which());
}

bool Vertex::is_null() const
{
    return is_null_;
}

bool Vertex::operator==( const Vertex& v ) const
{
    return is_null_ == v.is_null_ && union_ == v.union_;
}

bool Vertex::operator!=( const Vertex& v ) const
{
    return !operator==( v );
}

bool Vertex::operator<( const Vertex& v ) const
{
    if (is_null_ && v.is_null_) return false;
    return is_null_ != v.is_null_ ? is_null_ < v.is_null_ : union_ < v.union_;
}

const Road::Graph* Vertex::road_graph() const
{
    if ( is_null_ || union_.which() != 0 ) {
        return 0;
    }
    using boost::get;
    return get<RoadVertex_>( union_ ).graph;
}

Road::Vertex Vertex::road_vertex() const
{
    if ( is_null_ || union_.which() != 0 ) {
        return Road::Vertex();
    }
    using boost::get;
    return get<RoadVertex_>( union_ ).vertex;
}

const PublicTransport::Graph* Vertex::pt_graph() const
{
    if ( is_null_ || union_.which() != 1 ) {
        return 0;
    }
    using boost::get;
    return get<PtVertex_>( union_ ).graph;
}

PublicTransport::Vertex Vertex::pt_vertex() const
{
    if ( is_null_ || union_.which() != 1 ) {
        return 0;
    }
    using boost::get;
    return get<PtVertex_>( union_ ).vertex;
}

Road::Node get_road_node( const Multimodal::Vertex& v )
{
    return (*v.road_graph())[v.road_vertex()];
}

PublicTransport::Stop get_pt_stop( const Multimodal::Vertex& v )
{
    return (*v.pt_graph())[v.pt_vertex()];
}

const POI* Vertex::poi() const
{
    if ( is_null_ || union_.which() != 2 ) {
        return 0;
    }
    using boost::get;
    return get<const POI*>(union_);
}

Edge::ConnectionType Edge::connection_type() const
{
    if ( source_.type() == Vertex::Road ) {
        if ( target_.type() == Vertex::Road ) {
            return Road2Road;
        }
        else if ( target_.type() == Vertex::PublicTransport ) {
            return Road2Transport;
        }
        else {
            return Road2Poi;
        }
    }
    else if ( source_.type() == Vertex::PublicTransport ) {
        if ( target_.type() == Vertex::Road ) {
            return Transport2Road;
        }
        else if ( target_.type() == Vertex::PublicTransport ) {
            return Transport2Transport;
        }
    }
    else {
        if ( target_.type() == Vertex::Road ) {
            return Poi2Road;
        }
    }

    return UnknownConnection;
}

unsigned Edge::traffic_rules() const
{
    if ( connection_type() == Multimodal::Edge::Transport2Transport ) {
        return TrafficRulePublicTransport;
    }

    return (*source_.road_graph())[road_edge()].traffic_rules();
}

bool Edge::operator==( const Multimodal::Edge& e ) const
{
    return source_ == e.source_ && target_ == e.target_ && road_edge_ == e.road_edge_;
}

bool Edge::operator!=( const Multimodal::Edge& e ) const
{
    return source_ != e.source_ || target_ != e.target_ || road_edge_ != e.road_edge_;
}

bool Edge::operator<( const Multimodal::Edge& e ) const
{
    return source_ == e.source_ ? 
        ( target_ == e.target_ ?
          road_edge_ < e.road_edge_
          : target_ < e.target_ )
        : source_ < e.source_;
}

VertexIterator::VertexIterator( const Multimodal::Graph& graph )
{
    graph_ = &graph;
    boost::tie( road_it_, road_it_end_ ) = boost::vertices( graph_->road() );
    pt_graph_it_ = graph_->public_transports().subset_begin();
    pt_graph_it_end_ = graph_->public_transports().subset_end();
    poi_it_ = graph_->pois().begin();
    poi_it_end_ = graph_->pois().end();

    // If we have at least one public transport network
    if ( pt_graph_it_ != pt_graph_it_end_ ) {
        boost::tie( pt_it_, pt_it_end_ ) = boost::vertices( *pt_graph_it_->second );
    }
}

void VertexIterator::to_end()
{
    BOOST_ASSERT( graph_ != 0 );
    road_it_ = road_it_end_;
    pt_graph_it_ = pt_graph_it_end_;
    pt_it_ = pt_it_end_;
    poi_it_ = poi_it_end_;
}

Vertex VertexIterator::dereference() const
{
    Vertex vertex;
    BOOST_ASSERT( graph_ != 0 );

    if ( road_it_ != road_it_end_ ) {
        vertex = Vertex( &graph_->road(), *road_it_ );
    }
    else {
        if ( pt_it_ != pt_it_end_ ) {
            vertex = Vertex( &*pt_graph_it_->second, *pt_it_ );
        }
        else {
            vertex = Vertex( &*poi_it_->second );
        }
    }

    return vertex;
}

void VertexIterator::increment()
{
    BOOST_ASSERT( graph_ != 0 );

    if ( road_it_ != road_it_end_ ) {
        road_it_++;
    }
    else {
        if ( pt_it_ != pt_it_end_ ) {
            pt_it_++;
        }

        if ( pt_graph_it_ == pt_graph_it_end_ ) {
            if ( poi_it_ != poi_it_end_ ) {
                poi_it_++;
            }
        }

        while ( pt_it_ == pt_it_end_ && pt_graph_it_ != pt_graph_it_end_ ) {
            pt_graph_it_++;

            if ( pt_graph_it_ != pt_graph_it_end_ ) {
                // set on the beginning of the next pt_graph
                boost::tie( pt_it_, pt_it_end_ ) = boost::vertices( *pt_graph_it_->second );
            }
        }
    }
}

bool VertexIterator::equal( const VertexIterator& v ) const
{
    // not the same road graph
    if ( graph_ != v.graph_ ) {
        return false;
    }

    bool is_on_road = ( road_it_ != road_it_end_ );

    if ( is_on_road ) {
        return road_it_ == v.road_it_;
    }

    // not on the same pt graph
    if ( pt_graph_it_ != v.pt_graph_it_ ) {
        return false;
    }

    // else, same pt graph
    if ( pt_graph_it_ != pt_graph_it_end_ ) {
        return pt_it_ == v.pt_it_;
    }

    // else, on poi
    return poi_it_ == v.poi_it_;
}

// Road2Road
Edge::Edge( const Road::Graph* graph, const Road::Vertex& s, const Road::Vertex& t ) :
    source_( graph, s ), target_( graph, t )
{
    bool found = false;
    tie( road_edge_, found ) = edge( s, t, *graph );
    BOOST_ASSERT( found );
}

// Road2Transport
Edge::Edge( const Road::Graph* graph, const Road::Vertex& s,
            const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& t ) :
    source_( graph, s ), target_( pt_graph, t )
{
    using boost::target;
    const PublicTransport::Stop& stop = (*pt_graph)[t];
    if ( stop.opposite_road_edge() && s == target( stop.road_edge(), *graph ) ) {
        road_edge_ = *stop.opposite_road_edge();
    }
    else {
        road_edge_ = stop.road_edge();
    }
}

// Road2Poi
Edge::Edge( const Road::Graph* graph, const Road::Vertex& s,
            const POI* t ) :
    source_( graph, s ), target_( t )
{
    using boost::target;
    if ( t->opposite_road_edge() && s == target( t->road_edge(), *graph ) ) {
        road_edge_ = *t->opposite_road_edge();
    }
    else {
        road_edge_ = t->road_edge();
    }
}

// Transport2Road
Edge::Edge( const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& s,
            const Road::Graph* graph, const Road::Vertex& t ) :
    source_( pt_graph, s), target_( graph, t )
{
    using boost::source;
    const PublicTransport::Stop& stop = (*pt_graph)[s];
    if ( stop.opposite_road_edge() && t == source( stop.road_edge(), *graph ) ) {
        road_edge_ = *stop.opposite_road_edge();
    }
    else {
        road_edge_ = stop.road_edge();
    }
}

// Transport2Transport
Edge::Edge( const PublicTransport::Graph* pt_graph, const PublicTransport::Vertex& s, const PublicTransport::Vertex& t ) :
    source_( pt_graph, s ), target_( pt_graph, t )
{
    BOOST_ASSERT( edge( s, t, *pt_graph ).second );
    // arbitrary direction
    road_edge_ = (*pt_graph)[s].road_edge();
}

// Poi2Road
Edge::Edge( const POI* s,
            const Road::Graph* graph, const Road::Vertex& t ) :
    source_( s ), target_( graph, t )
{
    using boost::source;
    if ( s->opposite_road_edge() && t == source( s->road_edge(), *graph ) ) {
        road_edge_ = *s->opposite_road_edge();
    }
    else {
        road_edge_ = s->road_edge();
    }
}

Edge::Edge( const Vertex& s, const Vertex& t )
{
    using boost::target;
    using boost::source;
    using boost::edge;
    BOOST_ASSERT( (s.type() == Vertex::Road && t.type() == Vertex::Road) ||
                  (s.type() == Vertex::Road && t.type() == Vertex::PublicTransport) ||
                  (s.type() == Vertex::PublicTransport && t.type() == Vertex::Road) ||
                  (s.type() == Vertex::PublicTransport && t.type() == Vertex::PublicTransport) ||
                  (s.type() == Vertex::Road && t.type() == Vertex::Poi) ||
                  (s.type() == Vertex::Poi && t.type() == Vertex::Road) );

    source_ = s;
    target_ = t;
    if ( s.type() == Vertex::Road ) {
        if ( t.type() == Vertex::Road ) {
            bool found = false;
            tie(road_edge_,found) = edge( s.road_vertex(), t.road_vertex(), *s.road_graph() );
            BOOST_ASSERT( found );
        }
        else if (t.type() == Vertex::PublicTransport ) {
            const PublicTransport::Stop& stop = (*t.pt_graph())[t.pt_vertex()];
            if ( stop.opposite_road_edge() && s.road_vertex() == target( stop.road_edge(), *s.road_graph() ) ) {
                road_edge_ = *stop.opposite_road_edge();
            }
            else {
                road_edge_ = stop.road_edge();
            }
        }
        else if (t.type() == Vertex::Poi ) {
            if ( t.poi()->opposite_road_edge() && s.road_vertex() == target( t.poi()->road_edge(), *s.road_graph() ) ) {
                road_edge_ = *t.poi()->opposite_road_edge();
            }
            else {
                road_edge_ = t.poi()->road_edge();
            }
        }
    }
    else if ( s.type() == Vertex::PublicTransport ) {
        if ( t.type() == Vertex::Road ) {
            const PublicTransport::Stop& stop = (*s.pt_graph())[s.pt_vertex()];
            if ( stop.opposite_road_edge() && t.road_vertex() == source( stop.road_edge(), *t.road_graph() ) ) {
                road_edge_ = *stop.opposite_road_edge();
            }
            else {
                road_edge_ = stop.road_edge();
            }
        }
        else if (t.type() == Vertex::PublicTransport ) {
            BOOST_ASSERT( edge( s.pt_vertex(), t.pt_vertex(), *s.pt_graph() ).second );
            // arbitrary direction
            road_edge_ = (*s.pt_graph())[s.pt_vertex()].road_edge();
        }
    }
    else if ( s.type() == Vertex::Poi ) {
        if ( t.type() == Vertex::Road ) {
            if ( s.poi()->opposite_road_edge() && t.road_vertex() == source( s.poi()->road_edge(), *t.road_graph() ) ) {
                road_edge_ = *s.poi()->opposite_road_edge();
            }
            else {
                road_edge_ = s.poi()->road_edge();
            }
        }
    }
}

EdgeIterator::EdgeIterator( const Multimodal::Graph& graph )
{
    graph_ = &graph;
    boost::tie( vi_, vi_end_ ) = vertices( graph );

    if ( vi_ == vi_end_ ) {
        return;
    }

    // move to the first vertex with some out edges
    do {
        boost::tie( ei_, ei_end_ ) = out_edges( *vi_, *graph_ );

        if ( ei_ == ei_end_ ) {
            vi_++;
        }
    }
    while ( ei_ == ei_end_ && vi_ != vi_end_ );
}

void EdgeIterator::to_end()
{
    BOOST_ASSERT( graph_ != 0 );
    vi_ = vi_end_;
    ei_ = ei_end_;
    BOOST_ASSERT( vi_ == vi_end_ );
    BOOST_ASSERT( ei_ == ei_end_ );
}

Multimodal::Edge EdgeIterator::dereference() const
{
    BOOST_ASSERT( graph_ != 0 );
    BOOST_ASSERT( ei_ != ei_end_ );
    return *ei_;
}

void EdgeIterator::increment()
{
    BOOST_ASSERT( graph_ != 0 );

    if ( ei_ != ei_end_ ) {
        ei_++;
    }

    while ( ei_ == ei_end_ && vi_ != vi_end_ ) {
        vi_++;

        if ( vi_ != vi_end_ ) {
            boost::tie( ei_, ei_end_ ) = out_edges( *vi_, *graph_ );
        }
    }
}

bool EdgeIterator::equal( const EdgeIterator& v ) const
{
    // not the same road graph
    if ( graph_ != v.graph_ ) {
        return false;
    }

    if ( vi_ != vi_end_ ) {
        return vi_ == v.vi_ && ei_ == v.ei_;
    }

    return v.vi_ == v.vi_end_;
}

OutEdgeIterator::OutEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source )
{
    graph_ = &graph;
    source_ = source;

    if ( source.type() == Vertex::Road ) {
        boost::tie( road_it_, road_it_end_ ) = boost::out_edges( source.road_vertex(), graph_->road() );
    }
    else if ( source.type() == Vertex::PublicTransport ) {
        boost::tie( pt_it_, pt_it_end_ ) = boost::out_edges( source.pt_vertex(), *source.pt_graph() );
    }

    stop2road_connection_ = 0;
    road2stop_connection_ = 0;
    road2poi_connection_ = 0;
    poi2road_connection_ = 0;
}

void OutEdgeIterator::to_end()
{
    pt_it_ = pt_it_end_;
    road_it_ = road_it_end_;
    stop2road_connection_ = 2;
    road2stop_connection_ = -1;
    road2poi_connection_ = -1;
    poi2road_connection_ = 2;
}

Multimodal::Edge OutEdgeIterator::dereference() const
{
    Multimodal::Edge edge;
    BOOST_ASSERT( graph_ != 0 );

    if ( source_.type() == Vertex::Road ) {
        BOOST_ASSERT( road_it_ != road_it_end_ );
        Road::Vertex r_target = boost::target( *road_it_, graph_->road() );

        if ( road2stop_connection_ >= 0 && ( size_t )road2stop_connection_ < graph_->road()[ *road_it_ ].stops().size() ) {
            size_t idx = road2stop_connection_;
            const PublicTransport::Graph* pt_graph = graph_->road()[ *road_it_ ].stops()[ idx ]->graph();
            PublicTransport::Vertex v = *(graph_->road()[ *road_it_ ].stops()[ idx ]->vertex());
            edge = Multimodal::Edge( source_.road_graph(), source_.road_vertex(), pt_graph, v );
        }
        else if ( road2poi_connection_ >= 0 && ( size_t )road2poi_connection_ < graph_->road()[ *road_it_ ].pois().size() ) {
            size_t idx = road2poi_connection_;
            const POI* poi = graph_->road()[ *road_it_ ].pois()[ idx ];
            edge = Multimodal::Edge( source_.road_graph(), source_.road_vertex(), poi );
        }
        else {
            edge = Multimodal::Edge( source_.road_graph(), source_.road_vertex(), r_target );
        }
    }
    else if ( source_.type() == Vertex::PublicTransport ) {
        const PublicTransport::Graph* pt_graph = source_.pt_graph();
        PublicTransport::Vertex r_source = source_.pt_vertex();
        const PublicTransport::Stop *stop = &(*pt_graph)[r_source];

        if ( stop2road_connection_ == 0 ) {
            edge = Multimodal::Edge( pt_graph, r_source, &graph_->road(), target( stop->road_edge(), graph_->road() ) );
        }
        // if there is an opposite road edge attached
        else if ( stop2road_connection_ == 1 && stop->opposite_road_edge() ) {
            edge = Multimodal::Edge( pt_graph, r_source, &graph_->road(), target( *stop->opposite_road_edge(), graph_->road() ) );
        }
        else {
            PublicTransport::Vertex r_target = boost::target( *pt_it_, *pt_graph );
            edge = Multimodal::Edge( pt_graph, r_source, r_target );
        }
    }
    else if ( source_.type() == Vertex::Poi ) {
        if ( poi2road_connection_ == 0 ) {
            edge = Multimodal::Edge( source_.poi(), &graph_->road(), boost::target( source_.poi()->road_edge(), graph_->road() ) );
        }
        // if there is an opposite road edge attached
        else if ( poi2road_connection_ == 1 &&
                  source_.poi()->opposite_road_edge() ) {
            edge = Multimodal::Edge( source_.poi(), &graph_->road(), boost::target( *source_.poi()->opposite_road_edge(), graph_->road() ) );
        }
    }

    return edge;
}

void OutEdgeIterator::increment()
{
    BOOST_ASSERT( graph_ != 0 );

    if ( source_.type() == Vertex::Road ) {
        if ( road2stop_connection_ >= 0 && ( size_t )road2stop_connection_ < graph_->road()[*road_it_].stops().size() ) {
            road2stop_connection_++;
        }
        else if ( road2poi_connection_ >= 0 && ( size_t )road2poi_connection_ < graph_->road()[*road_it_].pois().size() ) {
            road2poi_connection_++;
        }
        else {
            road2stop_connection_ = 0;
            road2poi_connection_ = 0;
            road_it_++;
        }
    }
    else if ( source_.type() == Vertex::PublicTransport ) {
        const PublicTransport::Graph* pt_graph = source_.pt_graph();
        PublicTransport::Vertex r_source = source_.pt_vertex();
        const PublicTransport::Stop *stop = &(*pt_graph)[r_source];

        if ( stop2road_connection_ == 0 ) {
            // if there is an opposite edge
            if ( stop->opposite_road_edge() ) {
                stop2road_connection_ ++;
            }
            // else, step to 2
            else {
                stop2road_connection_ = 2;
            }
        }
        else if ( stop2road_connection_ == 1 ) {
            stop2road_connection_ ++;
        }
        else {
            bool is_at_pt_end = ( pt_it_ == pt_it_end_ );

            if ( !is_at_pt_end ) {
                pt_it_++;
            }
        }
    }
    else if ( source_.type() == Vertex::Poi ) {
        if ( poi2road_connection_ == 0 ) {
            // if there is an opposite edge
            if ( source_.poi()->opposite_road_edge() ) {
                poi2road_connection_ ++;
            }
            else {
                poi2road_connection_ = 2;
            }
        }
        else if ( poi2road_connection_ == 1 ) {
            poi2road_connection_ ++;
        }
    }
}

bool OutEdgeIterator::equal( const OutEdgeIterator& v ) const
{
    // not the same road graph
    if ( graph_ != v.graph_ ) {
        return false;
    }

    if ( source_ != v.source_ ) {
        return false;
    }

    if ( source_.type() == Vertex::Road ) {
        if ( road_it_ == road_it_end_ ) {
            return v.road_it_ == v.road_it_end_;
        }

        return road_it_ == v.road_it_ && road2stop_connection_ == v.road2stop_connection_ && road2poi_connection_ == v.road2poi_connection_;
    }

    if ( source_.type() == Vertex::PublicTransport ) {
        if ( stop2road_connection_ != v.stop2road_connection_ ) {
            return false;
        }

        return pt_it_ == v.pt_it_;
    }

    // else
    return poi2road_connection_ == v.poi2road_connection_;
}


InEdgeIterator::InEdgeIterator( const Multimodal::Graph& graph, Multimodal::Vertex source )
{
    graph_ = &graph;
    source_ = source;

    if ( source.type() == Vertex::Road ) {
        boost::tie( road_it_, road_it_end_ ) = boost::in_edges( source.road_vertex(), graph_->road() );
    }
    else if ( source.type() == Vertex::PublicTransport ) {
        boost::tie( pt_it_, pt_it_end_ ) = boost::in_edges( source.pt_vertex(), *source.pt_graph() );
    }

    stop_from_road_connection_ = 0;
    road_from_stop_connection_ = 0;
    road_from_poi_connection_ = 0;
    poi_from_road_connection_ = 0;
}

void InEdgeIterator::to_end()
{
    pt_it_ = pt_it_end_;
    road_it_ = road_it_end_;
    stop_from_road_connection_ = 2;
    road_from_stop_connection_ = -1;
    road_from_poi_connection_ = -1;
    poi_from_road_connection_ = 2;
}

Multimodal::Edge InEdgeIterator::dereference() const
{
    Multimodal::Edge edge;
    BOOST_ASSERT( graph_ != 0 );

    if ( source_.type() == Vertex::Road ) {
        BOOST_ASSERT( road_it_ != road_it_end_ );

        if ( road_from_stop_connection_ >= 0 && ( size_t )road_from_stop_connection_ < graph_->road()[ *road_it_ ].stops().size() ) {
            size_t idx = road_from_stop_connection_;
            const PublicTransport::Graph* pt_graph = graph_->road()[ *road_it_ ].stops()[ idx ]->graph();
            PublicTransport::Vertex v = *(graph_->road()[ *road_it_ ].stops()[ idx ]->vertex());
            edge = Multimodal::Edge( pt_graph, v, source_.road_graph(), source_.road_vertex() );
        }
        else if ( road_from_poi_connection_ >= 0 && ( size_t )road_from_poi_connection_ < graph_->road()[ *road_it_ ].pois().size() ) {
            size_t idx = road_from_poi_connection_;
            const POI* poi = graph_->road()[ *road_it_ ].pois()[ idx ];
            edge = Multimodal::Edge( poi, source_.road_graph(), source_.road_vertex() );
        }
        else {
            Road::Vertex r_source = source( *road_it_, graph_->road() );
            edge = Multimodal::Edge( source_.road_graph(), r_source, source_.road_vertex() );
        }
    }
    else if ( source_.type() == Vertex::PublicTransport ) {
        const PublicTransport::Graph* pt_graph = source_.pt_graph();
        PublicTransport::Vertex r_source = source_.pt_vertex();
        const PublicTransport::Stop *stop = &(*pt_graph)[r_source];

        if ( stop_from_road_connection_ == 0 ) {
            edge = Multimodal::Edge( &graph_->road(), source( stop->road_edge(), graph_->road() ), pt_graph, r_source );
        }
        // if there is an opposite road edge attached
        else if ( stop_from_road_connection_ == 1 && stop->opposite_road_edge() ) {
            edge = Multimodal::Edge( &graph_->road(), source( *stop->opposite_road_edge(), graph_->road() ), pt_graph, r_source );
        }
        else {
            PublicTransport::Vertex r_target = source( *pt_it_, *pt_graph );
            edge = Multimodal::Edge( pt_graph, r_target, r_source );
        }
    }
    else if ( source_.type() == Vertex::Poi ) {
        if ( poi_from_road_connection_ == 0 ) {
            edge = Multimodal::Edge( &graph_->road(), source( source_.poi()->road_edge(), graph_->road() ), source_.poi() );
        }
        // if there is an opposite road edge attached
        else if ( poi_from_road_connection_ == 1 &&
                  source_.poi()->opposite_road_edge() ) {
            edge = Multimodal::Edge( &graph_->road(), source( *source_.poi()->opposite_road_edge(), graph_->road() ), source_.poi() );
        }
    }

    return edge;
}

void InEdgeIterator::increment()
{
    BOOST_ASSERT( graph_ != 0 );

    if ( source_.type() == Vertex::Road ) {
        if ( road_from_stop_connection_ >= 0 && ( size_t )road_from_stop_connection_ < graph_->road()[*road_it_].stops().size() ) {
            road_from_stop_connection_++;
        }
        else if ( road_from_poi_connection_ >= 0 && ( size_t )road_from_poi_connection_ < graph_->road()[*road_it_].pois().size() ) {
            road_from_poi_connection_++;
        }
        else {
            road_from_stop_connection_ = 0;
            road_from_poi_connection_ = 0;
            road_it_++;
        }
    }
    else if ( source_.type() == Vertex::PublicTransport ) {
        const PublicTransport::Graph* pt_graph = source_.pt_graph();
        PublicTransport::Vertex r_source = source_.pt_vertex();
        const PublicTransport::Stop *stop = &(*pt_graph)[r_source];

        if ( stop_from_road_connection_ == 0 ) {
            // if there is an opposite edge
            if ( stop->opposite_road_edge() ) {
                stop_from_road_connection_ ++;
            }
            // else, step to 2
            else {
                stop_from_road_connection_ = 2;
            }
        }
        else if ( stop_from_road_connection_ == 1 ) {
            stop_from_road_connection_ ++;
        }
        else {
            bool is_at_pt_end = ( pt_it_ == pt_it_end_ );

            if ( !is_at_pt_end ) {
                pt_it_++;
            }
        }
    }
    else if ( source_.type() == Vertex::Poi ) {
        if ( poi_from_road_connection_ == 0 ) {
            // if there is an opposite edge
            if ( source_.poi()->opposite_road_edge() ) {
                poi_from_road_connection_ ++;
            }
            else {
                poi_from_road_connection_ = 2;
            }
        }
        else if ( poi_from_road_connection_ == 1 ) {
            poi_from_road_connection_ ++;
        }
    }
}

bool InEdgeIterator::equal( const InEdgeIterator& v ) const
{
    // not the same road graph
    if ( graph_ != v.graph_ ) {
        return false;
    }

    if ( source_ != v.source_ ) {
        return false;
    }

    if ( source_.type() == Vertex::Road ) {
        if ( road_it_ == road_it_end_ ) {
            return v.road_it_ == v.road_it_end_;
        }

        return road_it_ == v.road_it_ && road_from_stop_connection_ == v.road_from_stop_connection_ && road_from_poi_connection_ == v.road_from_poi_connection_;
    }

    if ( source_.type() == Vertex::PublicTransport ) {
        if ( stop_from_road_connection_ != v.stop_from_road_connection_ ) {
            return false;
        }

        return pt_it_ == v.pt_it_;
    }

    // else
    return poi_from_road_connection_ == v.poi_from_road_connection_;
}

VertexIndexProperty get( boost::vertex_index_t, const Multimodal::Graph& graph )
{
    return VertexIndexProperty( graph );
}

size_t get( const VertexIndexProperty& p, const Multimodal::Vertex& v )
{
    return p.get_index( v );
}

size_t VertexIndexProperty::get_index( const Vertex& v ) const
{
    // Maps a vertex to an integer in (0, num_vertices-1).
    //
    // If the vertex is a road vertex, maps it to its vertex_index in the road graph
    // If it is a public transport vertex, maps it to its vertex_index in the pt graph
    // and adds the number of vertices of the road graph and all the preceding public transport
    // graphs.
    // If it is a poi, maps it to its vertex index + the sum of num_vertices for all previous graphs.
    //
    // The complexity is then not constant (linear in terms of number of public transports graphs and pois).


    switch (v.type()) {
    case Vertex::Road: {
        const Vertex::RoadVertex_& rv = boost::get<Vertex::RoadVertex_>( v.union_ );
        return boost::get( boost::get( boost::vertex_index, *rv.graph ), rv.vertex );
    }

    case Vertex::PublicTransport: {
        size_t n = num_vertices( graph_.road() );

        const Vertex::PtVertex_& rv = boost::get<Vertex::PtVertex_>( v.union_ );
        for ( Multimodal::Graph::PublicTransportGraphList::const_iterator it = graph_.public_transports().begin();
              it != graph_.public_transports().end(); it++ ) {
            if ( &*it->second != rv.graph ) {
                n += num_vertices( *it->second );
            }
            else {
                n += boost::get( boost::get( boost::vertex_index, *rv.graph ), rv.vertex );
                break;
            }
        }
        return n;
    }

    case Vertex::Poi: {
        size_t n = num_vertices( graph_.road() );

        for ( Multimodal::Graph::PublicTransportGraphList::const_iterator it = graph_.public_transports().begin();
              it != graph_.public_transports().end(); it++ ) {
            n += num_vertices( *it->second );
        }

        // FIXME : could be sped up by using a POI index in a POI vector rather than a loop
        const POI* poi = boost::get<const POI*>( v.union_ );
        for ( Graph::PoiList::const_iterator it = graph_.pois().begin(); it != graph_.pois().end(); it++ ) {
            if ( &*it->second == poi ) {
                break;
            }

            n++;
        }
        return n;
    }
    }

    return 0;
}

size_t num_vertices( const Graph& graph )
{
    //
    // The number of vertices of a Multimodal::Graph is :
    // num_vertices of the road graph
    // + num_vertices of each pt graph
    // + the number of pois
    size_t n = 0;
    Multimodal::Graph::PublicTransportGraphList::const_subset_iterator it;

    for ( it = graph.public_transports().subset_begin(); it != graph.public_transports().subset_end(); it++ ) {
        n += num_vertices( *it->second );
    }

    return n + num_vertices( graph.road() ) + graph.pois().size();
}
size_t num_edges( const Graph& graph )
{
    size_t n = 0;
    VertexIterator vi, vi_end;
    for ( boost::tie( vi, vi_end ) = vertices( graph ); vi != vi_end; ++vi ) {
        n += out_degree( *vi, graph );
    }
    return n;
}
Vertex source( const Edge& e, const Graph& )
{
    return e.source();
}
Vertex target( const Edge& e, const Graph& )
{
    return e.target();
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

pair<InEdgeIterator, InEdgeIterator> in_edges( const Vertex& v, const Graph& graph )
{
    InEdgeIterator v_begin( graph, v ), v_end( graph, v );
    v_end.to_end();
    return std::make_pair( v_begin, v_end );
}

size_t out_degree( const Vertex& v, const Graph& graph )
{
    if ( v.type() == Vertex::Road ) {
        size_t sum = 0;
        Road::OutEdgeIterator ei, ei_end;

        //
        // For each out edge of the vertex, we have access to
        // N stops (given by stops.size())
        // M POis (given by pois.size())
        // the out edge (+1)
        for ( boost::tie( ei, ei_end ) = out_edges( v.road_vertex(), graph.road() ); ei != ei_end; ei++ ) {
            sum += graph.road()[ *ei ].stops().size() + graph.road()[ *ei ].pois().size() + 1;
        }

        return sum;
    }
    else if ( v.type() == Vertex::PublicTransport ) {
        //
        // For a PublicTransport::Vertex, we have access to 1 or 2 additional node (the road node)
        return out_degree( v.pt_vertex(), *v.pt_graph() ) + 
            ( (*v.pt_graph())[v.pt_vertex()].opposite_road_edge() ? 2 : 1);
    }

    // else (Poi), 1 road node is reachable
    return v.poi()->opposite_road_edge() ? 2 : 1;
}

size_t in_degree( const Vertex& v, const Graph& graph )
{
    if ( v.type() == Vertex::Road ) {
        size_t sum = 0;
        Road::InEdgeIterator ei, ei_end;

        //
        // For each in edge of the vertex, we have access to
        // N stops (given by stops.size())
        // M POis (given by pois.size())
        // the out edge (+1)
        for ( boost::tie( ei, ei_end ) = in_edges( v.road_vertex(), graph.road() ); ei != ei_end; ei++ ) {
            sum += graph.road()[ *ei ].stops().size() + graph.road()[ *ei ].pois().size() + 1;
        }

        return sum;
    }
    else if ( v.type() == Vertex::PublicTransport ) {
        //
        // For a PublicTransport::Vertex, we have access to 1 or 2 additional node (the road node)
        return in_degree( v.pt_vertex(), *v.pt_graph() ) + 
            ( (*v.pt_graph())[v.pt_vertex()].opposite_road_edge() ? 2 : 1);
    }

    // else (Poi), 1 road node is reachable
    return v.poi()->opposite_road_edge() ? 2 : 1;
}

size_t degree( const Vertex& v, const Graph& graph )
{
    return in_degree( v, graph ) + out_degree( v, graph );
}

std::pair< Edge, bool> edge( const Vertex& u, const Vertex& v, const Graph& graph )
{
    Multimodal::Edge e;
    Multimodal::EdgeIterator ei, ei_end;
    bool found = false;

    for ( boost::tie( ei, ei_end ) = edges( graph ); ei != ei_end; ei++ ) {
        if ( source( *ei, graph ) == u && target( *ei, graph ) == v ) {
            found = true;
            e = *ei;
            break;
        }
    }

    return std::make_pair( e, found );
}

std::pair<PublicTransport::Edge, bool> public_transport_edge( const Multimodal::Edge& e )
{
    if ( e.connection_type() != Multimodal::Edge::Transport2Transport ) {
        return std::make_pair( PublicTransport::Edge(), false );
    }

    PublicTransport::Edge ret_edge;
    bool found;
    boost::tie( ret_edge, found ) = edge( e.source().pt_vertex(), e.target().pt_vertex(), *( e.source().pt_graph() ) );
    return std::make_pair( ret_edge, found );
}


boost::optional<TransportMode> Graph::transport_mode( db_id_t id ) const
{
    TransportModes::const_iterator fit = transport_modes_.find( id );
    if ( fit == transport_modes_.end() ) {
        return boost::optional<TransportMode>();
    }
    return fit->second;
}

boost::optional<TransportMode> Graph::transport_mode( const std::string& name ) const
{
    NameToId::const_iterator fit = transport_mode_from_name_.find( name );
    if ( fit == transport_mode_from_name_.end() ) {
        return boost::optional<TransportMode>();
    }
    return transport_modes_.find(fit->second)->second;
}

void Graph::set_transport_modes( const TransportModes& tm )
{
    transport_modes_ = tm;
    // cache name to id
    transport_mode_from_name_.clear();
    for ( TransportModes::const_iterator it = transport_modes_.begin(); it != transport_modes_.end(); ++it ) {
        transport_mode_from_name_[ it->second.name() ] = it->first;
    }
}

Graph::Graph( std::auto_ptr<Road::Graph> r )
{
    set_road( r );
}

const Road::Graph& Graph::road() const {
    BOOST_ASSERT( road_.get() );
    return *road_;
}

Road::Graph& Graph::road() {
    BOOST_ASSERT( road_.get() );
    return *road_;
}

void Graph::set_road( std::auto_ptr<Road::Graph> r )
{
    // move
    road_ = r;
}

boost::optional<const PublicTransport::Network&> Graph::network( db_id_t id ) const
{
    NetworkMap::const_iterator it = network_map_.find(id);
    if ( it == network_map_.end() ) {
        return boost::optional<const PublicTransport::Network&>();
    }
    return it->second;
}

boost::optional<const PublicTransport::Graph&> Graph::public_transport( db_id_t id ) const
{
    PublicTransportGraphList::const_iterator it = public_transports_.find(id);
    if ( it == public_transports_.end() ) {
        return boost::optional<const PublicTransport::Graph&>();
    }
    return *it->second;
}

void Graph::set_public_transports( boost::ptr_map<db_id_t, PublicTransport::Graph>& nmap )
{
    for ( PublicTransportGraphList::iterator it = public_transports_.begin(); it != public_transports_.end(); ++it ) {
        delete it->second;
    }
    public_transports_.clear();
    // move

    while (!nmap.empty()) {
        boost::ptr_map<db_id_t, PublicTransport::Graph>::iterator it = nmap.begin();
		db_id_t id = it->first;
        PublicTransport::Graph* p(nmap.release(it).release());
        public_transports_[id] = p;
    }

    //
    // By default, all public transport networks are part of the selected subset
    public_transports_.select_all();
}

void Graph::select_public_transports( const std::set<db_id_t> & s )
{
    public_transports_.select(s);
}

std::set<db_id_t> Graph::public_transport_selection() const
{
    return public_transports_.selection();
}

Graph::~Graph()
{
    for ( PublicTransportGraphList::iterator it = public_transports_.begin(); it != public_transports_.end(); ++it ) {
        delete it->second;
    }
}

boost::optional<const POI&> Graph::poi( db_id_t id ) const
{
    PoiList::const_iterator it = pois_.find(id);
    if ( it == pois_.end() ) {
        return boost::optional<const POI&>();
    }
    return *it->second;
}

void Graph::set_pois( PoiList& p )
{
    pois_.clear();
    pois_.transfer( p );
}

ostream& operator<<( ostream& out, const Multimodal::Vertex& v )
{
    if ( v.type() == Multimodal::Vertex::Road ) {
        out << "R" << ( *v.road_graph() )[v.road_vertex()].db_id();
    }
    else if ( v.type() == Multimodal::Vertex::PublicTransport ) {
        out << "PT" << ( *v.pt_graph() )[v.pt_vertex()].db_id();
    }
    else if ( v.type() == Multimodal::Vertex::Poi ) {
        out << "POI" << v.poi()->db_id();
    }

    return out;
}

ostream& operator<<( ostream& out, const Multimodal::Edge& e )
{
    switch ( e.connection_type() ) {
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
    case Multimodal::Edge::Road2Poi:
        out << "Road2Poi ";
        break;
    case Multimodal::Edge::Poi2Road:
        out << "Poi2Road ";
        break;
    case Multimodal::Edge::UnknownConnection:
        throw std::runtime_error( "bug: should not reach here" );
    }

    out << "(" << e.source() << "," << e.target() << ")";
    return out;
}

std::ostream& operator<<( std::ostream& ostr, const Multimodal::VertexIterator& it )
{
    ostr << "{ graph(" << it.graph_ << "), vertex(" << *it << ") }";
    return ostr;
}

std::ostream& operator<<( std::ostream& ostr, const Multimodal::OutEdgeIterator& it )
{
    ostr << "out_edge_iterator{ source(" << it.source_ << "), graph(" << it.graph_;
    ostr << "), road_it(";

    if ( it.road_it_ == it.road_it_end_ ) {
        ostr << "-END-";
    }
    else {
        ostr << *it.road_it_;
    }

    ostr << "), pt_it(";

    if ( it.pt_it_ == it.pt_it_end_ ) {
        ostr << "-END-";
    }
    else {
        ostr << *it.pt_it_;
    }

    ostr << "), stop2road(" << it.stop2road_connection_ << "), road2stop(" << it.road2stop_connection_;
    ostr << "), poi2road(" << it.poi2road_connection_ << "), road2poi(" << it.road2poi_connection_;
    ostr << ")}" ;
    return ostr;
}

std::ostream& operator<<( std::ostream& ostr, const Multimodal::InEdgeIterator& it )
{
    ostr << "in_edge_iterator{ source(" << it.source_ << "), graph(" << it.graph_;
    ostr << "), road_it(";

    if ( it.road_it_ == it.road_it_end_ ) {
        ostr << "-END-";
    }
    else {
        ostr << *it.road_it_;
    }

    ostr << "), pt_it(";

    if ( it.pt_it_ == it.pt_it_end_ ) {
        ostr << "-END-";
    }
    else {
        ostr << *it.pt_it_;
    }

    ostr << "), stop_from_road(" << it.stop_from_road_connection_ << "), road_from_stop(" << it.road_from_stop_connection_;
    ostr << "), poi_from_road(" << it.poi_from_road_connection_ << "), road_from_poi(" << it.road_from_poi_connection_;
    ostr << ")}" ;
    return ostr;
}

std::ostream& operator<<( std::ostream& ostr, const Multimodal::EdgeIterator& it )
{
    ostr << "edge_iterator{";
    ostr << "vi(" << it.vi_;
    ostr << "), ei(" << it.ei_;
    ostr << ")}" ;
    return ostr;
}

} // namespace Multimodal

class coordinates_visitor
{
public:
    Point3D operator()(const Road::Graph& g, const Road::Vertex& v ) const
    {
        return g[v].coordinates();
    }
    
    Point3D operator()(const PublicTransport::Graph& g, const PublicTransport::Vertex& v ) const
    {
        return g[v].coordinates();
    }

    Point3D operator()(const POI& p) const
    {
        return p.coordinates();
    }
};

Point3D Multimodal::Vertex::coordinates() const
{
    return apply_visitor_<Point3D, coordinates_visitor>( coordinates_visitor() );
}

}
