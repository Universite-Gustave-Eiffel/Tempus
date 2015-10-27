#ifndef CH_CONTRACTION_HH
#define CH_CONTRACTION_HH

#include <chrono>
#include <boost/graph/adjacency_list.hpp>
#include "base.hh"

template<class It>
boost::iterator_range<It> pair_range(std::pair<It, It> const& p)
{
    return boost::make_iterator_range(p.first, p.second);
}

inline std::chrono::milliseconds mstime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}


struct VertexProperty
{
    Tempus::db_id_t id;
    uint32_t order;
};

struct EdgeProperty
{
    int weight;
};


typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> CHGraph;
typedef typename boost::graph_traits<CHGraph>::vertex_descriptor CHVertex;
typedef typename boost::graph_traits<CHGraph>::edge_descriptor CHEdge;

using TCost = int;
using TTargetSet = std::set<CHVertex>;
using TNodeId64 = Tempus::db_id_t;

struct TEdge
{
    CHVertex from;
    CHVertex to;
    TCost cost;
};


struct Shortcut
{
    CHVertex from;
    CHVertex to;
    TCost cost;
    CHVertex contracted;
};

std::vector<TEdge> get_contraction_shortcuts( const CHGraph& graph, CHVertex v );

void add_edge_or_update( CHGraph& graph, CHVertex u, CHVertex v, TCost cost );

std::map<CHVertex, TCost> witness_search(const CHGraph& graph, CHVertex contracted_node, CHVertex source, const TTargetSet& targets, TCost cutoff, int& search_space);


#endif
