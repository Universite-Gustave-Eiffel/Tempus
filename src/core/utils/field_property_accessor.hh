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

namespace Tempus {
///
/// A FieldPropertyAccessor implements a Readable Property Map concept and gives read access
/// to the member of a vertex or edge
template <class Graph, class Tag, class T, class Member>
struct FieldPropertyAccessor {
    typedef T value_type;
    typedef T& reference;
    typedef typename Tempus::vertex_or_edge<Graph, Tag>::descriptor key_type;
    typedef boost::readable_property_map_tag category;

    FieldPropertyAccessor( Graph& graph, Member mem ) : graph_( graph ), mem_( mem ) {}
    Graph& graph_;
    Member mem_;
};
}

namespace boost {
///
/// Implementation of FieldPropertyAccessor
template <class Graph, class Tag, class T, class Member>
T get( Tempus::FieldPropertyAccessor<Graph, Tag, T, Member> pmap, typename Tempus::vertex_or_edge<Graph, Tag>::descriptor e )
{
    return pmap.graph_[e].*( pmap.mem_ );
}
}


