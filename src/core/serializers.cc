/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
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

#include "serializers.hh"

#include <fstream>

#include "point.hh"
#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "multimodal_graph.hh"

namespace Tempus
{

void serialize( std::ostream& ostr, const char* ptr, size_t s, binary_serialization_t )
{
    ostr.write( ptr, s );
}
void unserialize( std::istream& istr, char* ptr, size_t s, binary_serialization_t )
{
    istr.read( ptr, s );
}

template <typename T>
void serialize( std::ostream& ostr, const Optional<T>& o, binary_serialization_t t )
{
    serialize( ostr, (const char*)&o, sizeof(o), t );
}
template <typename T>
void unserialize( std::istream& istr, Optional<T>& o, binary_serialization_t t )
{
    unserialize( istr, (char*)&o, sizeof(o), t );
}

void serialize( std::ostream& ostr, const Road::Edge& o, binary_serialization_t t )
{
    serialize( ostr, (const char*)&o, sizeof(o), t );
}
void unserialize( std::istream& istr, Road::Edge& o, binary_serialization_t t )
{
    unserialize( istr, (char*)&o, sizeof(o), t );
}

void serialize( std::ostream& ostr, const Abscissa& o, binary_serialization_t t )
{
    serialize( ostr, (const char*)&o, sizeof(o), t );
}
void unserialize( std::istream& istr, Abscissa& o, binary_serialization_t t )
{
    unserialize( istr, (char*)&o, sizeof(o), t );
}

void serialize( std::ostream& ostr, const POI::PoiType& poit, binary_serialization_t t )
{
    uint8_t p = poit;
    serialize( ostr, (const char*)&p, 1, t );
}
void unserialize( std::istream& istr, POI::PoiType& poit, binary_serialization_t t )
{
    uint8_t p;
    unserialize( istr, (char*)&p, 1, t );
    poit = POI::PoiType(p);
}

void serialize( std::ostream& ostr, const TransportModeSpeedRule& e, binary_serialization_t t )
{
    uint8_t v = e;
    serialize( ostr, (const char*)&v, 1, t );
}
void unserialize( std::istream& istr, TransportModeSpeedRule& e, binary_serialization_t t )
{
    uint8_t v;
    unserialize( istr, (char*)&v, 1, t );
    e = TransportModeSpeedRule(v);
}
void serialize( std::ostream& ostr, const TransportModeEngine& e, binary_serialization_t t )
{
    uint8_t v = e;
    serialize( ostr, (const char*)&v, 1, t );
}
void unserialize( std::istream& istr, TransportModeEngine& e, binary_serialization_t t )
{
    uint8_t v;
    unserialize( istr, (char*)&v, 1, t );
    e = TransportModeEngine(v);
}
void serialize( std::ostream& ostr, const TransportModeRouteType& e, binary_serialization_t t )
{
    uint8_t v = e;
    serialize( ostr, (const char*)&v, 1, t );
}
void unserialize( std::istream& istr, TransportModeRouteType& e, binary_serialization_t t )
{
    uint8_t v;
    unserialize( istr, (char*)&v, 1, t );
    e = TransportModeRouteType(v);
}


void serialize( std::ostream& ostr, const std::string& t, binary_serialization_t )
{
    size_t s = t.size();
    ostr.write( (const char*)&s, sizeof(s) );
    ostr.write( t.c_str(), t.size() );
}

void unserialize( std::istream& istr, std::string& t, binary_serialization_t )
{
    size_t s;
    istr.read( (char*)&s, sizeof(s) );
    std::string str;
    str.resize(s);
    istr.read( (char*)&str[0], s );
    t = std::move(str);
}

void serialize( std::ostream& ostr, const Point2D& pt, binary_serialization_t )
{
    float f[2];
    f[0] = pt.x();
    f[1] = pt.y();
    ostr.write( (const char*)f, sizeof(float)*2 );
}

void unserialize( std::istream& istr, Point2D& pt, binary_serialization_t )
{
    float f[2];
    istr.read( (char*)f, sizeof(float)*2 );
    pt.set_x( f[0] );
    pt.set_y( f[1] );
}

void serialize( std::ostream& ostr, const Point3D& pt, binary_serialization_t )
{
    float f[3];
    f[0] = pt.x();
    f[1] = pt.y();
    f[2] = pt.z();
    ostr.write( (const char*)f, sizeof(float)*3 );
}

void unserialize( std::istream& istr, Point3D& pt, binary_serialization_t )
{
    float f[3];
    istr.read( (char*)f, sizeof(float)*3 );
    pt.set_x( f[0] );
    pt.set_y( f[1] );
    pt.set_z( f[2] );
}

void serialize( std::ostream& ostr, const Road::Graph& graph, binary_serialization_t )
{
    uint32_t cs = graph.m_forward.m_column.size();
    ostr.write( (const char*)&cs, sizeof(cs) );
    ostr.write( (const char*) &graph.m_forward.m_column[0], sizeof(decltype(graph.m_forward.m_column)::value_type) * cs );

    uint32_t rs = graph.m_forward.m_rowstart.size();
    ostr.write( (const char*)&rs, sizeof(rs) );
    ostr.write( (const char*) &graph.m_forward.m_rowstart[0], sizeof(decltype(graph.m_forward.m_rowstart)::value_type) * rs );

    uint32_t vs = graph.m_vertex_properties.size();
    ostr.write( (const char*)&vs, sizeof(vs) );
    ostr.write( (const char*) &graph.m_vertex_properties[0], sizeof(decltype(graph.m_vertex_properties)::value_type) * vs );

    uint32_t ps = graph.m_forward.m_edge_properties.size();
    ostr.write( (const char*)&ps, sizeof(ps) );
    ostr.write( (const char*) &graph.m_forward.m_edge_properties[0], sizeof(decltype(graph.m_forward.m_edge_properties)::value_type) * ps );
}

void unserialize( std::istream& istr, Road::Graph& graph, binary_serialization_t )
{
    uint32_t cs;
    istr.read( (char*)&cs, sizeof(cs) );
    graph.m_forward.m_column.resize(cs);
    istr.read( (char*) &graph.m_forward.m_column[0], sizeof(decltype(graph.m_forward.m_column)::value_type) * cs );

    uint32_t rs;
    istr.read( (char*)&rs, sizeof(rs) );
    graph.m_forward.m_rowstart.resize(rs);
    istr.read( (char*) &graph.m_forward.m_rowstart[0], sizeof(decltype(graph.m_forward.m_rowstart)::value_type) * rs );

    uint32_t vs;
    istr.read( (char*)&vs, sizeof(vs) );
    graph.m_vertex_properties.resize(vs);
    istr.read( (char*) &graph.m_vertex_properties[0], sizeof(decltype(graph.m_vertex_properties)::value_type) * vs );

    uint32_t ps;
    istr.read( (char*)&ps, sizeof(ps) );
    graph.m_forward.m_edge_properties.resize(ps);
    istr.read( (char*) &graph.m_forward.m_edge_properties[0], sizeof(decltype(graph.m_forward.m_edge_properties)::value_type) * ps );
}

void serialize( std::ostream& ostr, const PublicTransport::Stop& stop, binary_serialization_t t )
{
    db_id_t db_id = stop.db_id();
    serialize( ostr, db_id, t );
    serialize( ostr, stop.graph_, t );
    serialize( ostr, stop.vertex_, t );
    serialize( ostr, stop.name_, t );
    serialize( ostr, stop.is_station_, t );
    serialize( ostr, stop.parent_station_, t );
    serialize( ostr, stop.road_edge_, t );
    serialize( ostr, stop.opposite_road_edge_, t );
    serialize( ostr, stop.abscissa_road_section_, t );
    serialize( ostr, stop.zone_id_, t );
    serialize( ostr, stop.coordinates_, t );
}

void unserialize( std::istream& istr, PublicTransport::Stop& stop, binary_serialization_t t )
{
    db_id_t db_id;
    unserialize( istr, db_id, t );
    stop.set_db_id( db_id );
    unserialize( istr, stop.graph_, t );
    unserialize( istr, stop.vertex_, t );
    unserialize( istr, stop.name_, t );
    unserialize( istr, stop.is_station_, t );
    unserialize( istr, stop.parent_station_, t );
    unserialize( istr, stop.road_edge_, t );
    unserialize( istr, stop.opposite_road_edge_, t );
    unserialize( istr, stop.abscissa_road_section_, t );
    unserialize( istr, stop.zone_id_, t );
    unserialize( istr, stop.coordinates_, t );
}

void serialize( std::ostream& ostr, const PublicTransport::Section& section, binary_serialization_t t )
{
    serialize( ostr, section.network_id(), t );
}

void unserialize( std::istream& istr, PublicTransport::Section& section, binary_serialization_t t )
{
    db_id_t db_id;
    unserialize( istr, db_id, t );
    section.set_network_id(db_id);
}

void serialize( std::ostream& ostr, const PublicTransport::Graph& graph, binary_serialization_t t )
{
    // vertices
    size_t n_vertices = num_vertices( graph );
    serialize( ostr, n_vertices, t );
    PublicTransport::VertexIterator it, it_end;
    for ( boost::tie(it, it_end) = vertices( graph ); it != it_end; it++ ) {
        serialize( ostr, graph[*it], t );
    }

    // edges
    size_t n_edges = num_edges( graph );
    serialize( ostr, n_edges, t );
    PublicTransport::EdgeIterator eit, eit_end;
    for ( boost::tie(eit, eit_end) = edges( graph ); eit != eit_end; eit++ ) {
        PublicTransport::Vertex v1, v2;
        v1 = source( *eit, graph );
        v2 = target( *eit, graph );
        serialize( ostr, v1, t );
        serialize( ostr, v2, t );
        serialize( ostr, graph[*eit], t );
    }
}

void unserialize( std::istream& istr, PublicTransport::Graph& graph, binary_serialization_t t )
{
    // vertices
    size_t n_vertices;
    unserialize( istr, n_vertices, t );
    for ( size_t i = 0; i < n_vertices; i++ ) {
        PublicTransport::Vertex v = add_vertex( graph );
        unserialize( istr, graph[v], t );
    }

    // edges
    size_t n_edges;
    unserialize( istr, n_edges, t );
    for ( size_t i = 0; i < n_edges; i++ ) {
        PublicTransport::Vertex v1, v2;
        unserialize( istr, v1, t );
        unserialize( istr, v2, t );
        PublicTransport::Edge e;
        bool is_added = false;
        boost::tie( e, is_added ) = add_edge( v1, v2, graph );
        BOOST_ASSERT( is_added );
        unserialize( istr, graph[e], t );
    }
}

void serialize( std::ostream& ostr, const Multimodal::Graph::StopIndex& s, binary_serialization_t t )
{
    serialize( ostr, s.graph(), t );
    serialize( ostr, s.vertex(), t );
}

void unserialize( std::istream& istr, Multimodal::Graph::StopIndex& s, binary_serialization_t t )
{
    PublicTransportGraphIndex idx;
    PublicTransport::Vertex v;
    unserialize( istr, idx, t );
    unserialize( istr, v, t );
    s = Multimodal::Graph::StopIndex( idx, v );
}

void serialize( std::ostream& ostr, const POI& poi, binary_serialization_t t )
{
    serialize( ostr, poi.poi_type(), t );
    serialize( ostr, poi.name(), t );
    serialize( ostr, poi.parking_transport_modes(), t );
    serialize( ostr, poi.road_edge(), t );
    serialize( ostr, poi.opposite_road_edge(), t );
    serialize( ostr, poi.abscissa_road_section(), t );
    serialize( ostr, poi.coordinates(), t );
}

void unserialize( std::istream& istr, POI& poi, binary_serialization_t t )
{
    unserialize( istr, poi.poi_type_, t );
    unserialize( istr, poi.name_, t );
    unserialize( istr, poi.parking_transport_modes_, t );
    unserialize( istr, poi.road_edge_, t );
    unserialize( istr, poi.opposite_road_edge_, t );
    unserialize( istr, poi.abscissa_road_section_, t );
    unserialize( istr, poi.coordinates_, t );
}

void serialize( std::ostream& ostr, const PublicTransport::Agency& agency, binary_serialization_t t )
{
    serialize( ostr, agency.db_id(), t );
    serialize( ostr, agency.name(), t );
}

void unserialize( std::istream& istr, PublicTransport::Agency& agency, binary_serialization_t t )
{
    db_id_t id;
    unserialize( istr, id, t );
    std::string name;
    unserialize( istr, name, t );
    agency.set_db_id( id );
    agency.set_name( std::move(name) );
}

void serialize( std::ostream& ostr, const PublicTransport::Network& network, binary_serialization_t t )
{
    serialize( ostr, network.agencies(), t );
}

void unserialize( std::istream& istr, PublicTransport::Network network, binary_serialization_t t )
{
    std::vector<PublicTransport::Agency> agencies;
    unserialize( istr, agencies, t );
    for ( auto agency : agencies ) {
        network.add_agency( agency );
    }
}

void serialize( std::ostream& ostr, const TransportMode& tm, binary_serialization_t t )
{
    serialize( ostr, tm.db_id(), t );
    serialize( ostr, tm.name_, t );
    serialize( ostr, tm.is_public_transport_, t );
    serialize( ostr, tm.need_parking_, t );
    serialize( ostr, tm.is_shared_, t );
    serialize( ostr, tm.must_be_returned_, t );
    serialize( ostr, tm.traffic_rules_, t );
    serialize( ostr, tm.speed_rule_, t );
    serialize( ostr, tm.toll_rules_, t );
    serialize( ostr, tm.engine_type_, t );
    serialize( ostr, tm.route_type_, t );
}

void unserialize( std::istream& istr, TransportMode& tm, binary_serialization_t t )
{
    db_id_t db_id;
    unserialize( istr, db_id, t );
    tm.set_db_id(db_id);
    unserialize( istr, tm.name_, t );
    unserialize( istr, tm.is_public_transport_, t );
    unserialize( istr, tm.need_parking_, t );
    unserialize( istr, tm.is_shared_, t );
    unserialize( istr, tm.must_be_returned_, t );
    unserialize( istr, tm.traffic_rules_, t );
    unserialize( istr, tm.speed_rule_, t );
    unserialize( istr, tm.toll_rules_, t );
    unserialize( istr, tm.engine_type_, t );
    unserialize( istr, tm.route_type_, t );
}

void serialize( std::ostream& ostr, const Multimodal::Graph& graph, binary_serialization_t t )
{
    // metadata
    serialize( ostr, graph.metadata(), t );
    // transport mode
    serialize( ostr, graph.transport_modes(), t );
    // road
    serialize( ostr, graph.road(), t );
    // pois
    serialize( ostr, graph.pois(), t );
    // pt networks
    serialize( ostr, graph.network_map(), t );
    // pt graphs
    serialize( ostr, graph.public_transports().size(), t );
    for ( auto p : graph.public_transports() ) {
        // id
        serialize( ostr, p.first, t );
        // graph
        serialize( ostr, *p.second, t );
    }
    // pt graph selection
    serialize( ostr, graph.public_transport_selection(), t );
    // edge_pois
    serialize( ostr, graph.road_edge_pois_, t );
    // edge stops
    serialize( ostr, graph.road_edge_stops_, t );
}

void unserialize( std::istream& istr, Multimodal::Graph& graph, binary_serialization_t t )
{
    // metadata
    unserialize( istr, graph.metadata_, t );
    // transport mode
    unserialize( istr, graph.transport_modes_, t );
    // road
    std::unique_ptr<Road::Graph> rd( new Road::Graph() );
    unserialize( istr, *rd, t );
    graph.set_road( std::move( rd ) );
    // pois
    unserialize( istr, graph.pois_, t );
    // pt networks
    unserialize( istr, graph.network_map_, t );
    // pt graphs
    size_t nb_pt_graphs = 0;
    unserialize( istr, nb_pt_graphs, t );
    std::map<db_id_t, std::unique_ptr<PublicTransport::Graph>> pt_graphs;
    for ( size_t i = 0; i < nb_pt_graphs; i++ ) {
        // id
        db_id_t id;
        unserialize( istr, id, t );
        // graph
        std::unique_ptr<PublicTransport::Graph> g( new PublicTransport::Graph() );
        unserialize( istr, *g, t );
        pt_graphs.insert( std::make_pair(id, std::move(g)) );
    }
    graph.set_public_transports( std::move(pt_graphs) );
    // pt graph selection
    unserialize( istr, graph.selected_transport_graphs_, t );
    // edge_pois
    unserialize( istr, graph.road_edge_pois_, t );
    // edge stops
    unserialize( istr, graph.road_edge_stops_, t );
}

}
