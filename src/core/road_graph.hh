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

#ifndef TEMPUS_ROAD_GRAPH_HH
#define TEMPUS_ROAD_GRAPH_HH

#include "common.hh"
#include <boost/graph/adjacency_list.hpp>

namespace Tempus {
// forward declaration
namespace PublicTransport {
struct Stop;
}
struct POI;

/**
   A Road::Graph is made of Road::Node and Road::Section

   It generally maps to the database's schema: one class exists for each table.
   Tables with 1<->N arity are represented by STL containers (vectors or lists)
   External keys are represented by reference to other classes or by vertex/edge descriptors

   Road::Node and Road::Section classes are used to build a BGL road graph as "bundled" edge and vertex properties
*/
namespace Road {
///
/// Storage types used to make a road graph
typedef boost::vecS VertexListType;
typedef boost::vecS EdgeListType;

///
/// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
/// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
typedef boost::mpl::if_<boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
/// see adjacency_list.hpp
typedef boost::detail::edge_desc_impl<boost::directed_tag, Vertex> Edge;

struct Node;
struct Section;
///
/// The final road graph type
typedef boost::adjacency_list<VertexListType, EdgeListType, boost::directedS, Node, Section > Graph;

///
/// Used as Vertex.
/// Refers to the 'road_node' DB's table
struct Node : public Base {
    /// This is a shortcut to the vertex index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's vertex from a Node.
    /// Can be null
    DECLARE_RW_PROPERTY( vertex, OrNull<Vertex> );

    /// Total number of incident edges > 2
    DECLARE_RW_PROPERTY( is_bifurcation, bool );

    /// Type of parking available on this node
    /// This is to be used for parking on the streets
    /// Special parks are represented using a POI
    /// This is a bitfield value composeed of TransportModeTrafficRules
    DECLARE_RW_PROPERTY( parking_traffic_rules, int );

public:
    Node() : is_bifurcation_(false), parking_traffic_rules_(0) {}
};

///
/// Used as Directed Edge.
/// Refers to the 'road_section' DB's table
struct Section : public Base {
    /// This is a shortcut to the edge index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's edge from a Section.
    /// Can be null
    DECLARE_RW_PROPERTY( edge, Edge );

    DECLARE_RW_PROPERTY( road_type, db_id_t );

    DECLARE_RW_PROPERTY( traffic_rules, int );
    DECLARE_RW_PROPERTY( length, double );
    DECLARE_RW_PROPERTY( car_speed_limit, double );
    DECLARE_RW_PROPERTY( lane, int );
    DECLARE_RW_PROPERTY( is_roundabout, bool );
    DECLARE_RW_PROPERTY( is_bridge, bool );
    DECLARE_RW_PROPERTY( is_tunnel, bool );
    DECLARE_RW_PROPERTY( is_ramp, bool );
    DECLARE_RW_PROPERTY( is_tollway, bool );

public:
    Section() : traffic_rules_(0), length_(0.0), car_speed_limit_(0.0) {}
    ///
    /// List of public transport stops, attached to this road section
    /// FIXME turn into private
    std::vector< PublicTransport::Stop* > stops;
    ///
    /// List of Point Of Interests attached to this road section
    /// FIXME turn into private
    std::vector<POI*> pois;
};

typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;

///
/// refers to the 'road_restriction_time_penalty' DB's table
/// This structure reflects turn restrictions on the road network
class Restriction : public Base {
public:
    ///
    /// Map of cost associated for each transport type
    /// the key is here a binary combination of TransportTrafficRule
    /// the value is a cost, as double (including infinity)
    typedef std::map<int, double> CostPerTransport;

    typedef std::vector<Edge> EdgeSequence;

    Restriction( db_id_t id, const EdgeSequence& edge_seq, const CostPerTransport& cost = CostPerTransport() ) :
        Base( id ),
        road_edges_( edge_seq ),
        cost_per_transport_( cost )
    {}

    DECLARE_RO_PROPERTY( road_edges, EdgeSequence );

    DECLARE_RW_PROPERTY( cost_per_transport, CostPerTransport );
};

class Restrictions {
public:
    Restrictions( const Graph& road_graph ) : road_graph_(&road_graph) {}

    ///
    /// List of restrictions (as edges)
    typedef std::list<Restriction> RestrictionSequence;

    DECLARE_RO_PROPERTY( restrictions, RestrictionSequence );

    ///
    /// Add a restriction
    /// edges is a sequence of Edge, considered here as non-oriented (from the database)
    void add_restriction( db_id_t id,
                          const Restriction::EdgeSequence& edges,
                          const Restriction::CostPerTransport& cost );
    void add_restriction( Restriction restriction ); 

private:
    ///
    /// Pointer to the underlying road graph
    const Graph* road_graph_;
};
}  // Road namespace

} // Tempus namespace

#endif
