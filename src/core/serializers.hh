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

#ifndef TEMPUS_GRAPH_SERIALIZERS_HH
#define TEMPUS_GRAPH_SERIALIZERS_HH

#include <iosfwd>

namespace Tempus
{

struct Point2D;
struct Point3D;
class TransportMode;

namespace Road
{
struct Node;
struct Section;
}
namespace PublicTransport
{
struct Stop;
struct Section;
}
namespace Multimodal
{
struct Graph;
}

struct binary_serialization_t {};

void serialize( std::ostream& ostr, const Point2D& pt, binary_serialization_t );
void unserialize( std::istream& istr, Point2D& pt, binary_serialization_t );

void serialize( std::ostream& ostr, const Point3D& pt, binary_serialization_t );
void unserialize( std::istream& istr, Point3D& pt, binary_serialization_t );

void serialize( std::ostream& ostr, const PublicTransport::Stop&, binary_serialization_t );
void unserialize( std::istream& istr, PublicTransport::Stop&, binary_serialization_t );

void serialize( std::ostream& ostr, const TransportMode&, binary_serialization_t );
void unserialize( std::istream& istr, TransportMode&, binary_serialization_t );

void serialize( std::ostream& ostr, const Multimodal::Graph&, binary_serialization_t );
void unserialize( std::istream& istr, Multimodal::Graph&, binary_serialization_t );

} // namespace Tempus

#endif
