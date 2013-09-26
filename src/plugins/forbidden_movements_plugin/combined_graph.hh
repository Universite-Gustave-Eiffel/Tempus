// (c) 2013 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License


template <class GVertex, class Automaton>
std::pair<typename Automaton::vertex_descriptor, bool> find_transition_( const Automaton& automaton,
                                                                         typename Automaton::vertex_descriptor q,
                                                                         GVertex u,
                                                                         GVertex v )
{
    BGL_FORALL_OUTEDGES_T( q, edge, automaton, Automaton ) {
        if ( automaton[ edge ].source == u &&
             automaton[ edge ].target == v ) {
            return std::make_pair( target( edge, automaton ), true );
        }
    }
    // not found
    return std::make_pair( q, false );
}

// this is the heart of the combination :
// for a graph vertex u with a state q and an edge to v
// look for a transition from q to qp given by (u,v) in the automaton
template <class GVertex, class Automaton>
typename Automaton::vertex_descriptor find_transition( const Automaton& automaton,
                                                       typename Automaton::vertex_descriptor q,
                                                       GVertex u,
                                                       GVertex v )
{
    typedef typename Automaton::vertex_descriptor AVertex;

    AVertex q0 = *(vertices( automaton ).first);
    AVertex qp = q0;
    // look for transition
    bool transition_found = false;
    boost::tie( qp, transition_found ) = find_transition_( automaton, q, u, v );
    if ( ! transition_found ) {
        // no transition found on q, trying on q0
        transition_found = false;
        boost::tie( qp, transition_found ) = find_transition_( automaton, q0, u, v );
    }
    return qp;
}


//
// This is an adaptor to make a Graph and an automaton of penalties behave like as boost::graph
// The current implementation brings the minimal set of methods required by Dijkstra
// (out_edges)
//
// Requirements on the automaton are :
// - the automaton is a graph
// - its edges must have a 'source' and a 'target' fields that are vertex descriptors of the (road) graph
//
template <class Graph,
          class Automaton>
class CombinedGraphAdaptor
{
public:

    typedef Graph graph_type;
    typedef Automaton automaton_type;
    typedef typename Automaton::vertex_descriptor AVertex;
    typedef typename Graph::vertex_descriptor GVertex;
    typedef typename Graph::out_edge_iterator GEdgeIterator;

    // types not used by dijkstra, but needed by compilation
    typedef void adjacency_iterator;
    typedef void in_edge_iterator;
    typedef void vertex_iterator;
    typedef void edge_iterator;
    typedef void vertices_size_type;
    typedef void edges_size_type;

    // graph traits :
    typedef std::pair< GVertex, AVertex >                     vertex_descriptor;
    typedef std::pair< vertex_descriptor, vertex_descriptor > edge_descriptor;
    typedef boost::directed_tag                               directed_category;
    typedef boost::disallow_parallel_edge_tag                 edge_parallel_category;
    typedef boost::incidence_graph_tag                        traversal_category;


    // IncidenceGraph
    typedef size_t       degree_size_type;


    class out_edge_iterator :
        public boost::iterator_facade< out_edge_iterator,
                                       edge_descriptor,
                                       boost::forward_traversal_tag,
                                       /*Reference*/ edge_descriptor>
    {
    public:
        out_edge_iterator() :
            graph_(0),
            automaton_(0),
            u(0),
            q(0)
        {}
        out_edge_iterator( const Graph& graph, const Automaton& automaton, vertex_descriptor source ) :
            graph_(&graph),
            automaton_(&automaton),
            u(source.first),
            q(source.second)
        {
            boost::tie( out_edge, out_edge_end ) = out_edges( u, *graph_ );
            update_edge();
        }

        edge_descriptor dereference() const
        {
            return edge_;
        }

        void update_edge()
        {
            GVertex v = target( *out_edge, *graph_ );
            AVertex qp = find_transition( *automaton_, q, u, v );
            edge_ = std::make_pair( std::make_pair( u, q ), std::make_pair( v, qp ) );
        }

        void increment()
        {
            if ( out_edge == out_edge_end ) {
                return;
            }

            out_edge++;
            if ( out_edge == out_edge_end ) {
                return;
            }
            update_edge();
        }
        bool equal( const out_edge_iterator& v ) const
        {
            if ( (graph_ != v.graph_) || (automaton_ != v.automaton_ ) ) {
                return false;
            }
            return q == v.q && u == v.u && out_edge == v.out_edge;
        }
        void to_end()
        {
            out_edge = out_edge_end;
        }
    private:
        const Graph* graph_;
        const Automaton* automaton_;
        
        AVertex q;
        GVertex u;
        GEdgeIterator out_edge, out_edge_end;
        edge_descriptor edge_;
    };

    CombinedGraphAdaptor( Graph& graph, Automaton& automaton ) :graph_(graph), automaton_(automaton) {}

    Graph graph_;
    Automaton automaton_;
};

template <class G, class A>
std::pair<typename CombinedGraphAdaptor<G,A>::out_edge_iterator,
          typename CombinedGraphAdaptor<G,A>::out_edge_iterator>
out_edges( const typename CombinedGraphAdaptor<G,A>::vertex_descriptor& v, const CombinedGraphAdaptor<G,A>& g )
{
    typedef typename CombinedGraphAdaptor<G,A>::out_edge_iterator oe_iterator;
    oe_iterator it( g.graph_, g.automaton_, v );
    oe_iterator it_end( g.graph_, g.automaton_, v );
    it_end.to_end();
    return std::make_pair( it, it_end );
}

//
// This is an adaptor class to offer an index property map for a combined graph
//
// Requirements on the CombinedGraph : the underlying Graph and Automaton must have a 'vertex_index' property
template <class CombinedGraph>
struct CombinedIndexMap
{
    typedef typename CombinedGraph::vertex_descriptor key_type;
    typedef unsigned long value_type;
    typedef value_type& reference;
    typedef boost::readable_property_map_tag category;

    CombinedIndexMap( const CombinedGraph& graph ) : graph_(graph) {}

    const CombinedGraph& graph_;
};

template <class CombinedGraph>
typename CombinedIndexMap<CombinedGraph>::value_type get( const CombinedIndexMap<CombinedGraph>& cmap,
                                                          const typename CombinedIndexMap<CombinedGraph>::key_type& k )
{
    unsigned long graph_idx = get( get( boost::vertex_index, cmap.graph_.graph_ ), k.first );
    unsigned long nvertices = num_vertices( cmap.graph_.automaton_ );
    unsigned long automaton_idx = get( get( boost::vertex_index, cmap.graph_.automaton_ ), k.second );
    // FIXME : this is a maximum here, considering each graph vertex may have nvertices different states
    // which is in general false. Could it cause too much memory to be allocated ??
    return graph_idx * nvertices + automaton_idx;
}

template <class G, class A>
CombinedIndexMap< CombinedGraphAdaptor<G,A> > get( boost::vertex_index_t, const CombinedGraphAdaptor<G,A>& g )
{
    return CombinedIndexMap< CombinedGraphAdaptor<G,A> >( g );
}

//
// Adaptor class to make a weightmap on a graph and a penalty map on an automaton behave
// like a weight map where the key is a pair of ( graph vertex, automaton state )
//
template <class CombinedGraph, class WeightMap, class PenaltyMap>
struct CombinedWeightMap
{
    typedef typename boost::graph_traits<typename CombinedGraph::graph_type>::edge_descriptor GEdge;
    typedef typename CombinedGraph::edge_descriptor key_type;
    typedef typename WeightMap::value_type value_type;
    typedef value_type& reference;
    typedef boost::readable_property_map_tag category;

    CombinedWeightMap( const CombinedGraph& graph, WeightMap& wmap, PenaltyMap& pmap ) :
        graph_(graph),
        wmap_(wmap),
        pmap_(pmap)
    {}

    value_type get( const key_type& k ) const
    {
        typedef typename CombinedGraph::vertex_descriptor Vertex;
        Vertex v1 = k.first;
        Vertex v2 = k.second;
        bool ok = false;
        GEdge e;
        boost::tie( e, ok ) = edge( v1.first, v2.first, graph_.graph_ );
        BOOST_ASSERT( ok );
        double w = boost::get( wmap_, e );
        double p = boost::get( pmap_, v1.second );
        //        std::cout << "weight " << e << " state= " << v2.second << " = " << w << " +" << p << std::endl;
        return w + p;
    }
private:
    const CombinedGraph& graph_;
    WeightMap& wmap_;
    PenaltyMap& pmap_;
};

template <class G, class W, class P>
typename CombinedWeightMap<G,W,P>::value_type get( const CombinedWeightMap<G,W,P>& p, const typename CombinedWeightMap<G,W,P>::key_type& k )
{
    return p.get( k );
}
