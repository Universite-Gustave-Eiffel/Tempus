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
    Vertex vertex;

    bool is_junction;
    bool is_bifurcation;
    
    int parking_transport_type;  ///< bitfield of TransportTypeId
};

///
/// Used as Directed Edge.
/// Refers to the 'road_section' DB's table
struct Section : public Base {

    Section() : transport_type(0), length(0.0), car_speed_limit(0.0), car_average_speed(0.0) {}
    /// This is a shortcut to the edge index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's edge from a Section.
    /// Can be null
    Edge edge;

    db_id_t       road_type;
    /// allowed transport types
    db_id_t       transport_type; ///< bitfield of TransportTypeId
    double        length;
    double        car_speed_limit;
    double        car_average_speed;
    int           lane;
    bool          is_roundabout;
    bool          is_bridge;
    bool          is_tunnel;
    bool          is_ramp;
    bool          is_tollway;

    ///
    /// List of public transport stops, attached to this road section
    std::vector< PublicTransport::Stop* > stops;
    ///
    /// List of Point Of Interests attached to this road section
    std::vector<POI*> pois;
};

typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;

///
/// refers to the 'road_restriction' DB's table
/// This structure reflects turn restrictions on the road network
class Restriction : public Base {
public:
    ///
    /// Map of cost associated for each transport type
    /// the key is here a bitfield (refer to TransportType)
    /// the value is a cost, as double (including infinity)
    typedef std::map<int, double> CostPerTransport;

    typedef std::vector<Edge> EdgeSequence;

    Restriction( db_id_t id, const EdgeSequence& edge_seq, const CostPerTransport& cost = CostPerTransport() ) :
        road_edges_( edge_seq ),
        cost_per_transport_( cost )
    {
        db_id = id;
    }

    const EdgeSequence& road_edges() const {
        return road_edges_;
    }

    const CostPerTransport& cost_per_transport() const {
        return cost_per_transport_;
    }
    CostPerTransport& cost_per_transport() {
        return cost_per_transport_;
    }
private:
    ///
    /// Array of road edges
    EdgeSequence road_edges_;

    CostPerTransport cost_per_transport_;
};

class Restrictions {
public:
    Restrictions( const Graph& road_graph ) : road_graph_(&road_graph) {}

    ///
    /// List of restrictions (as edges)
    typedef std::list<Restriction> RestrictionSequence;

    const RestrictionSequence& restrictions() const {
        return restrictions_;
    }

    ///
    /// Add a restriction
    /// edges is a sequence of Edge, considered here as non-oriented (from the database)
    void add_restriction( db_id_t id,
                          const Restriction::EdgeSequence& edges,
                          const Restriction::CostPerTransport& cost );

private:
    RestrictionSequence restrictions_;

    ///
    /// Pointer to the underlying road graph
    const Graph* road_graph_;
};
}  // Road namespace

///
/// refers to the 'poi' DB's table
struct POI : public Base {
    enum PoiType {
        TypeCarPark = 1,
        TypeSharedCarPoint,
        TypeCyclePark,
        TypeSharedCyclePoint,
        TypeUserPOI
    };
    int poi_type;

    std::string name;
    int parking_transport_type; ///< bitfield of TransportTypeId

    ///
    /// Link to a road edge
    Road::Edge road_edge;

    ///
    /// optional link to the opposite road edge
    /// considered null if == to road_edge
    Road::Edge opposite_road_edge;

    ///
    /// Number between 0 and 1 : position of the POI on the main road section
    double abscissa_road_section;
};
} // Tempus namespace

#endif
