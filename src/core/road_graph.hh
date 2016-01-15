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

#include <iosfwd>

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/graph/compressed_sparse_row_graph.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif
#include "common.hh"
#include "point.hh"
#include "serializers.hh"

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

struct Node;
struct Section;
///
/// The final road graph type
/// vertex and edge indices are stored in a uin32_t, which means there is maximum of 4G vertices or arcs
typedef boost::compressed_sparse_row_graph<boost::bidirectionalS, Node, Section, /*GraphProperty = */ boost::no_property, uint32_t, uint32_t> Graph;
typedef Graph::vertex_descriptor Vertex;
typedef Graph::edge_descriptor Edge;

/// Road types
enum RoadType
{
    RoadMotorway = 1,
    RoadPrimary = 2,
    RoadSecondary = 3,      
    RoadStreet = 4,
    RoadOther = 5,
    RoadCycleWay = 6,       //FIXME redundant with traffic rules ?
    RoadPedestrianOnly = 7  //FIXME redundant with traffic rules ?
};

///
/// Used as Vertex.
/// Refers to the 'road_node' DB's table
struct Node : public Base {

    /// Total number of incident edges > 2
    DECLARE_RW_PROPERTY( is_bifurcation, bool );

    /// coordinates
    DECLARE_RW_PROPERTY( coordinates, Point3D );
public:
    Node() : is_bifurcation_(false) {}
};

///
/// Used as Directed Edge.
/// Refers to the 'road_section' DB's table
struct Section : public Base {

    DECLARE_RW_PROPERTY( length, float );
    DECLARE_RW_PROPERTY( car_speed_limit, float );
    DECLARE_RW_PROPERTY( traffic_rules, uint16_t );

    /// Type of parking available on this section
    /// This is to be used for parking on the streets
    /// Special parks are represented using a POI
    /// This is a bitfield value composeed of TransportModeTrafficRules
    DECLARE_RW_PROPERTY( parking_traffic_rules, uint16_t );

    DECLARE_RW_PROPERTY( lane, uint8_t );

    bool is_roundabout() const { return (road_flags_ & RoadIsRoundAbout) != 0; }
    bool is_bridge() const { return (road_flags_ & RoadIsBridge) != 0; }
    bool is_tunnel() const { return (road_flags_ & RoadIsTunnel) != 0; }
    bool is_ramp() const { return (road_flags_ & RoadIsRamp) != 0; }
    bool is_tollway() const { return (road_flags_ & RoadIsTollway) != 0; }

    void set_is_roundabout(bool b) { road_flags_ |= b ? RoadIsRoundAbout : 0; }
    void set_is_bridge(bool b) { road_flags_ |= b ? RoadIsBridge : 0; }
    void set_is_tunnel(bool b) { road_flags_ |= b ? RoadIsTunnel : 0; }
    void set_is_ramp(bool b) { road_flags_ |= b ? RoadIsRamp : 0; }
    void set_is_tollway(bool b) { road_flags_ |= b ? RoadIsTollway : 0; }

    DECLARE_RW_PROPERTY( road_type, RoadType );

public:
    Section() : length_(0.0), car_speed_limit_(0.0), traffic_rules_(0), parking_traffic_rules_(0), road_flags_(0) {}

private:
    uint8_t road_flags_;
    enum RoadFlag
    {
        RoadIsRoundAbout = 1 << 0,
        RoadIsBridge =     1 << 1,
        RoadIsTunnel =     1 << 2,
        RoadIsRamp =       1 << 3,
        RoadIsTollway =    1 << 4
    };
};

typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;

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

void serialize( std::ostream& ostr, const Road::Graph&, binary_serialization_t );
void unserialize( std::istream& istr, Road::Graph&, binary_serialization_t );

} // Tempus namespace

namespace boost {
namespace detail {
std::ostream& operator<<( std::ostream& ostr, const Tempus::Road::Edge& e );
}
}

#endif
