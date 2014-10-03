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

#ifndef TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH
#define TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH

#include <boost/optional.hpp>
#include "common.hh"
#include "road_graph.hh"

namespace Tempus {
/**
   A PublicTransport::Graph is a made of PublicTransport::Stop and PublicTransport::Section

   It generally maps to the database's schema: one class exists for each table.
   Tables with 1<->N arity are represented by STL containers (vectors or lists)
   External keys are represented by pointers to other classes or by vertex/edge descriptors.

   PublicTransport::Stop and PublicTransport::Section classes are used to build a BGL public transport graph.
*/
namespace PublicTransport {

///
/// Public transport agency
class Agency : public Base
{
public:
    DECLARE_RW_PROPERTY( name, std::string );
};
typedef std::vector<Agency> AgencyList;

///
/// Public transport networks. A network can be made of several agencies
struct Network : public Base {
    DECLARE_RW_PROPERTY( name, std::string );

public:
    ///
    /// Get the list of agencies of this network
    const AgencyList& agencies() const { return agencies_; }

    ///
    /// Add an agency to this network
    void add_agency( const Agency& agency );
private:
    AgencyList agencies_;
};

///
/// storage types used to make a road graph
typedef boost::vecS VertexListType;
typedef boost::vecS EdgeListType;

///
/// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
/// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
typedef boost::mpl::if_<boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
/// see adjacency_list.hpp
typedef boost::detail::edge_desc_impl<boost::bidirectional_tag, Vertex> Edge;

struct Stop;
struct Section;
///
/// Definition of a public transport graph
typedef boost::adjacency_list<VertexListType, EdgeListType, boost::bidirectionalS, Stop, Section> Graph;

///
/// Used as a vertex in a PublicTransportGraph.
/// Refers to the 'pt_stop' DB's table
struct Stop : public Base {
public:
    Stop() : Base(), graph_(0) {}
 
    /// Shortcut to the public transport where this stop belongs
    /// Can be null
    DECLARE_RW_PROPERTY( graph, const Graph* );

    /// This is a shortcut to the vertex index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's vertex from a Node.
    /// Can be null
    DECLARE_RW_PROPERTY( vertex, boost::optional<Vertex> );

    DECLARE_RW_PROPERTY( name, std::string );
    DECLARE_RW_PROPERTY( is_station, bool );

    ///
    /// link to a possible parent station, or null
    DECLARE_RW_PROPERTY( parent_station, boost::optional<Vertex> );

    /// link to a road edge
    DECLARE_RW_PROPERTY( road_edge, Road::Edge );

    ///
    /// optional link to the opposite road edge
    /// Can be null
    DECLARE_RW_PROPERTY( opposite_road_edge, boost::optional<Road::Edge> );

    ///
    /// Number between 0 and 1 : position of the stop on the main road section
    DECLARE_RW_PROPERTY( abscissa_road_section, double );

    ///
    /// Fare zone ID of this stop
    DECLARE_RW_PROPERTY( zone_id, int );

    ///
    /// coordinates
    DECLARE_RW_PROPERTY( coordinates, Point3D );
};

///
/// used as an Edge in a PublicTransportGraph
struct Section {
public:
    Section() : graph_(0) {}

    /// Shortcut to the public transport graph where this edge belongs
    /// Can be null
    DECLARE_RW_PROPERTY( graph, const Graph* );

    /// This is a shortcut to the edge index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's edge from a Section
    /// Can be null
    DECLARE_RW_PROPERTY( edge, boost::optional<Edge> );

    /// must not be null
    DECLARE_RW_PROPERTY( network_id, db_id_t );
};

///
/// Convenience function - Get the departure stop of a public transport section
inline Stop get_stop_from( const Section& s )
{
    return (*s.graph())[source( *s.edge(), *s.graph() )];
}

///
/// Convenience function - Get the arrival stop of a public transport section
inline Stop get_stop_to( const Section& s )
{
    return (*s.graph())[target( *s.edge(), *s.graph() )];
}


typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;

} // PublicTransport namespace
} // Tempus namespace

#endif
