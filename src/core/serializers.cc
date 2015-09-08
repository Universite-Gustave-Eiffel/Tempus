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

#include "point.hh"
#include "road_graph.hh"

namespace Tempus
{

std::ostream& serialize( std::ostream& ostr, const Point2D& pt, binary_serialization_t )
{
    float f[2];
    f[0] = pt.x();
    f[1] = pt.y();
    ostr.write( (const char*)f, sizeof(float)*2 );
    return ostr;
}

void unserialize( std::istream& istr, Point2D& pt, binary_serialization_t )
{
    float f[2];
    istr.read( (char*)f, sizeof(float)*2 );
    pt.set_x( f[0] );
    pt.set_y( f[1] );
}

std::ostream& serialize( std::ostream& ostr, const Point3D& pt, binary_serialization_t )
{
    float f[3];
    f[0] = pt.x();
    f[1] = pt.y();
    f[2] = pt.z();
    ostr.write( (const char*)f, sizeof(float)*3 );
    return ostr;
}

void unserialize( std::istream& istr, Point3D& pt, binary_serialization_t )
{
    float f[3];
    istr.read( (char*)f, sizeof(float)*3 );
    pt.set_x( f[0] );
    pt.set_y( f[1] );
    pt.set_z( f[2] );
}

std::ostream& serialize( std::ostream& ostr, const Road::Node& node, binary_serialization_t t )
{
    char c = node.is_bifurcation() ? 1 : 0;
    ostr.write( &c, 1 );
    return serialize( ostr, node.coordinates(), t );
}

void unserialize( std::istream& istr, Road::Node& node, binary_serialization_t t )
{
    char c;
    istr.read( &c, 1 );
    node.set_is_bifurcation( c );
    unserialize( istr, node.coordinates_, t );
}

std::ostream& serialize( std::ostream& ostr, const Road::Section& section, binary_serialization_t )
{
    ostr.write( (const char*)&section.length_, sizeof(section.length_) );
    ostr.write( (const char*)&section.car_speed_limit_, sizeof(section.car_speed_limit_) );
    ostr.write( (const char*)&section.traffic_rules_, sizeof(section.traffic_rules_) );
    ostr.write( (const char*)&section.parking_traffic_rules_, sizeof(section.parking_traffic_rules_) );
    ostr.write( (const char*)&section.lane_, sizeof(section.lane_) );
    ostr.write( (const char*)&section.road_flags_, sizeof(section.road_flags_) );
    ostr.write( (const char*)&section.road_type_, sizeof(section.road_type_) );
    return ostr;
}

void unserialize( std::istream& istr, Road::Section& section, binary_serialization_t )
{
    istr.read( (char*)&section.length_, sizeof(section.length_) );
    istr.read( (char*)&section.car_speed_limit_, sizeof(section.car_speed_limit_) );
    istr.read( (char*)&section.traffic_rules_, sizeof(section.traffic_rules_) );
    istr.read( (char*)&section.parking_traffic_rules_, sizeof(section.parking_traffic_rules_) );
    istr.read( (char*)&section.lane_, sizeof(section.lane_) );
    istr.read( (char*)&section.road_flags_, sizeof(section.road_flags_) );
    istr.read( (char*)&section.road_type_, sizeof(section.road_type_) );
}

std::ostream& serialize( std::ostream& ostr, const Road::Graph& graph, binary_serialization_t )
{
    uint32_t cs = graph.m_forward.m_column.size();
    std::cout << "cs=" << cs << std::endl;
    ostr.write( (const char*)&cs, sizeof(cs) );
    ostr.write( (const char*) &graph.m_forward.m_column[0], sizeof(decltype(graph.m_forward.m_column)::value_type) * cs );

    uint32_t rs = graph.m_forward.m_rowstart.size();
    std::cout << "rs=" << rs << std::endl;
    ostr.write( (const char*)&rs, sizeof(rs) );
    ostr.write( (const char*) &graph.m_forward.m_rowstart[0], sizeof(decltype(graph.m_forward.m_rowstart)::value_type) * rs );

    uint32_t ps = graph.m_forward.m_edge_properties.size();
    std::cout << "ps=" << ps << std::endl;
    ostr.write( (const char*)&ps, sizeof(ps) );
    ostr.write( (const char*) &graph.m_forward.m_edge_properties[0], sizeof(decltype(graph.m_forward.m_edge_properties)::value_type) * ps );

    return ostr;
}

void unserialize( std::istream& istr, Road::Graph& graph, binary_serialization_t )
{
    uint32_t cs;
    istr.read( (char*)&cs, sizeof(cs) );
    std::cout << "cs=" << cs << std::endl;
    graph.m_forward.m_column.resize(cs);
    istr.read( (char*) &graph.m_forward.m_column[0], sizeof(decltype(graph.m_forward.m_column)::value_type) * cs );

    uint32_t rs;
    istr.read( (char*)&rs, sizeof(rs) );
    std::cout << "rs=" << rs << std::endl;
    graph.m_forward.m_rowstart.resize(rs);
    istr.read( (char*) &graph.m_forward.m_rowstart[0], sizeof(decltype(graph.m_forward.m_rowstart)::value_type) * rs );

    uint32_t ps;
    istr.read( (char*)&ps, sizeof(ps) );
    std::cout << "ps=" << ps << std::endl;
    graph.m_forward.m_edge_properties.resize(ps);
    istr.read( (char*) &graph.m_forward.m_edge_properties[0], sizeof(decltype(graph.m_forward.m_edge_properties)::value_type) * ps );
}

}
