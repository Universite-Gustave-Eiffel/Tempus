/**
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TEMPUS_CH_PREPROCESS_HH
#define TEMPUS_CH_PREPROCESS_HH

#include <vector>
#include <functional>
#include <boost/graph/adjacency_list.hpp>

#include "base.hh"

namespace Tempus
{

///
/// Updateable CH graph used for node ordering and contraction

struct VertexProperty
{
    db_id_t id;
    uint32_t order;
};

///
/// Type used for road cost
using TCost = int; // fixed point

struct EdgeProperty
{
    TCost weight;
};

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS,
                              boost::bidirectionalS,
                              VertexProperty,
                              EdgeProperty> CHGraph;

typedef typename boost::graph_traits<CHGraph>::vertex_descriptor CHVertex;
typedef typename boost::graph_traits<CHGraph>::edge_descriptor CHEdge;

///
/// The node ordering processing
/// \param[inout] graph The input graph that will be contracted
/// \param[in] node_id A function that maps a vertex to its id
/// \returns the ordered nodes
std::vector<CHVertex> order_graph( CHGraph& graph, std::function<db_id_t(CHVertex)> node_id );

struct Shortcut
{
    CHVertex from;
    CHVertex to;
    TCost cost;
    CHVertex contracted; // the "middle" node that has been used for the shortcut
};

///
/// The graph contraction processing
/// \param[inout] graph The input graph that will be contracted
/// \returns the shortcuts created
std::vector<Shortcut> contract_graph( CHGraph& graph );

}

#endif
