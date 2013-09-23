// Plugin sample on a road graph
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

#include <boost/format.hpp>
#if 0
#include <boost/graph/relax.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#endif
#include <boost/pending/indirect_cmp.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include <boost/graph/iteration_macros.hpp>
//#include <boost/pending/relaxed_heap.hpp>
//#include <boost/graph/dijkstra_shortest_paths.hpp>
//#include "custom_dijkstra.hpp"

#include "plugin.hh"
#include "multimodal_graph.hh"

using namespace std;

template <class UniquePairAssociativeContainer>
class associative_property_map_default_value
{
    typedef UniquePairAssociativeContainer C;
public:
    typedef typename C::key_type key_type;
    typedef typename C::value_type::second_type value_type;
    typedef value_type& reference;
    typedef boost::lvalue_property_map_tag category;

    associative_property_map_default_value( const value_type& defaultv ) : c_(0),
                                                                           default_(defaultv) {}
    associative_property_map_default_value( C& c, const value_type& defaultv ) : c_(&c),
                                                                                 default_(defaultv) {}

    value_type get( const key_type& k ) const
    {
        typename C::const_iterator it = c_->find( k );
        if ( it == c_->end() ) {
            return default_;
        }
        return it->second;
    }
    void put( const key_type& k, const value_type& v )
    {
        (*c_)[k] = v;
    }
private:
    C* c_;
    value_type default_;
};

template <class Map>
typename Map::value_type::second_type get( const associative_property_map_default_value<Map>& pa, const typename Map::key_type& k )
{
    return pa.get( k );
}

template <class Map>
void put( associative_property_map_default_value<Map>& pa, typename Map::key_type k, const typename Map::value_type::second_type& v )
{
    return pa.put( k, v );
}

template <class Graph,
          class Automaton,
          class DistanceMap,
          class PredecessorMap,
          class WeightMap,
          class PenaltyMap>
void mydijkstra(
                const Graph& graph,
                const Automaton& automaton,
                PenaltyMap penalty_map,
                typename boost::graph_traits<Graph>::vertex_descriptor start_vertex,
                typename boost::graph_traits<Graph>::vertex_descriptor target_vertex,
                PredecessorMap predecessor_map,
                DistanceMap distance_map,
                WeightMap weight_map
                )
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor GVertex;
    typedef typename boost::graph_traits<Automaton>::vertex_descriptor AVertex;

    // label type : graph vertex x automaton state (vertex)
    typedef std::pair<GVertex, AVertex> Label;

    // <=> get(dmap, a) < get(dmap, b)
    typedef boost::indirect_cmp< DistanceMap, std::less<double> > Cmp;
    //typedef IndirectCmp<DistanceMap> Cmp;
    Cmp cmp( distance_map );

    // priority queue for vertices
    // sort elements by distance_map[v]
    typedef boost::heap::d_ary_heap< Label,
                                     boost::heap::arity<4>,
                                     boost::heap::compare< Cmp >
                                     >
    VertexQueue;

    VertexQueue vertex_queue( cmp );

    double infinity = std::numeric_limits<double>::max();


    BGL_FORALL_VERTICES_T(current_vertex, graph, Graph) {
      // Default all vertex predecessors to the vertex itself
      put(predecessor_map, current_vertex, current_vertex);
    }


    AVertex q0 = *vertices( automaton ).first;
    Label l0 = std::make_pair( start_vertex, q0 );
    put( distance_map, l0, 0.0 );
    vertex_queue.push( l0 );
 
    while ( !vertex_queue.empty() ) {
        Label min_l = vertex_queue.top();

        GVertex u = min_l.first;
        if ( u == target_vertex ) {
            break;
        }
        AVertex q = min_l.second;

        vertex_queue.pop();

        double min_distance = get( distance_map, min_l );
        std::cout << "min_vertex: " << u << " state: " << q << " min_dist:" << min_distance << std::endl;

        typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
        BGL_FORALL_OUTEDGES_T(u, current_edge, graph, Graph) {

            GVertex v = target( current_edge, graph );

            // is an automaton transition has been found ?
            bool transition_found = false;
            AVertex qp;
#if 0
            BGL_FORALL_OUTEDGES_T( q, edge, automaton, Automaton ) {
                if ( automaton[ edge ].source == u &&
                     automaton[ edge ].target == target( current_edge, graph ) ) {
                    transition_found = true;
                    std::cout << "transition found " << edge << std::endl;
                    qp = target( edge, automaton );
                    break;
                }
            }
#endif

            if ( ! transition_found ) {
                // FIXME not always true
                qp = q0;
            }

            double c = get( weight_map, current_edge );
            if ( q != qp ) {
                c += get( penalty_map, q );
            }

            Label new_l = std::make_pair( v, qp );
            double pi_uq = get( distance_map, min_l );
            double pi_vqp = get( distance_map, new_l ) ;

            if ( pi_uq + c < pi_vqp ) {
                put( distance_map, new_l, pi_uq + c );
                put( predecessor_map, v, u );
                vertex_queue.push( new_l );
            }
        }
    }
}

namespace Tempus
{
    class ForbiddenMovementPlugin : public Plugin
    {
    public:

	ForbiddenMovementPlugin( Db::Connection& db ) : Plugin( "forbidden_movement_plugin", db )
	{
            declare_option("constrain", "Use forbidden edges", true );
	}

	virtual ~ForbiddenMovementPlugin()
	{
	}


    private:
        struct AutomatonNode
        {
            double penalty;
        };

        struct AutomatonEdge
        {
            Road::Vertex source, target;
            // road graph edge id;
            db_id_t edge_id;
        };

        // forbidden movement automata construction
        typedef boost::adjacency_list< boost::vecS, boost::vecS,
                                       boost::undirectedS,
                                       AutomatonNode,
                                       AutomatonEdge> Automaton;

        Automaton automaton_;

        Road::Vertex source_, destination_;

        class LengthCalculator
        {
        public:
            LengthCalculator() {}
            
            double operator()( Road::Graph& graph, Road::Edge& e )
            {
                
            }
        };

    public:

	virtual void post_build()
	{
	    graph_ = Application::instance()->graph();
            Road::Graph& road_graph = graph_.road;

            automaton_.clear();

            Automaton::vertex_descriptor v1, v2, v3;
            Automaton::edge_descriptor e1, e2;
            v1 = boost::add_vertex( automaton_ );
            automaton_[ v1 ].penalty = 0.0;
            v2 = boost::add_vertex( automaton_ );
            automaton_[ v2 ].penalty = 0.0;
            v3 = boost::add_vertex( automaton_ );
            automaton_[ v3 ].penalty = 1000.0;

            bool ok;
            boost::tie( e1, ok ) = boost::add_edge( v1, v2, automaton_ );
            BOOST_ASSERT( ok );
            boost::tie( e2, ok ) = boost::add_edge( v2, v3, automaton_ );
            BOOST_ASSERT( ok );

            // forbidden edges
            automaton_[ e1 ].edge_id = 14023;

            automaton_[ e1 ].source = vertex_from_id( 21652, road_graph );
            automaton_[ e1 ].target = vertex_from_id( 21712, road_graph );

            automaton_[ e2 ].edge_id = 34913;

            automaton_[ e2 ].source = vertex_from_id( 21712, road_graph );
            automaton_[ e2 ].target = vertex_from_id( 21691, road_graph );
	}

	virtual void pre_process( Request& /* request */ ) throw (std::invalid_argument)
	{
            // request is ignored for now
	    result_.clear();

            // source node fixed to 21687 (db's id)
            // target node : 22129

            Road::Graph& road_graph = graph_.road;
            source_ = vertex_from_id( 21687, road_graph );
            destination_ = vertex_from_id( 22129, road_graph );

            std::cout << "source: " << source_ << std::endl;
            std::cout << "destination: " << destination_ << std::endl;
	}

	virtual void road_vertex_accessor( Road::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
	    }
	}

	virtual void process()
	{
	    Road::Graph& road_graph = graph_.road;

	    std::vector<Road::Vertex> pred_map( boost::num_vertices(road_graph) );
            //	    std::vector<double> distance_map( boost::num_vertices(road_graph) );
            typedef std::pair< Road::Vertex, boost::graph_traits<Automaton>::vertex_descriptor > Label;
            std::map<Label, double> distance_map;
            associative_property_map_default_value< std::map<Label, double> > distance_pmap( distance_map, std::numeric_limits<double>::max() );

	    FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, double Road::Section::*> weight_map( road_graph, &Road::Section::length );


	    FieldPropertyAccessor<Automaton, boost::vertex_property_tag, double, double AutomatonNode::*> penalty_map( automaton_, &AutomatonNode::penalty );

	    Tempus::PluginRoadGraphVisitor vis( this );

            Road::Vertex start_vertex = source_;
            Road::Vertex target_vertex = destination_;


            
#if 0
            boost::custom_dijkstra_shortest_paths( road_graph,
                                                         start_vertex,
                                                         &pred_map[0],
                                                         &distance_map[0],
                                                         weight_map,
                                                         boost::get( boost::vertex_index, road_graph ),
                                                         std::less<double>(),
                                                         boost::closed_plus<double>(),
                                                         std::numeric_limits<double>::max(),
                                                         0.0,
                                                         vis
                                                         );
#else
            mydijkstra( road_graph,
                        automaton_,
                        penalty_map,
                        start_vertex,
                        target_vertex,
                        &pred_map[0],
                        distance_pmap,
                        weight_map );
#endif

            std::list<Road::Vertex> path;
            Road::Vertex current = target_vertex;
            bool found = true;
            while ( current != start_vertex )
            {
                path.push_front( current );
                if ( pred_map[current] == current )
                {
                    found = false;
                    break;
                }
                current = pred_map[ current ];
            }
            if ( !found )
            {
                cerr << "No path found !" << endl;
                return;
            }

	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();

            bool first_loop = true;
            Road::Vertex previous;
            for ( std::list<Road::Vertex>::iterator it = path.begin(); it != path.end(); it++ )
	    {
		Road::Vertex v = *it;
		
		// User-oriented roadmap
		if ( first_loop )
		{
		    previous = v;
		    first_loop = false;
		    continue;
		}

		// Find an edge, based on a source and destination vertex
		Road::Edge e;
		bool found = false;
		boost::tie( e, found ) = boost::edge( previous, v, road_graph );
		if ( !found )
		    continue;

                Roadmap::RoadStep* step = new Roadmap::RoadStep();
                roadmap.steps.push_back( step );
                step->road_section = e;
		step->costs[CostDistance] += road_graph[e].length;
		roadmap.total_costs[CostDistance] += road_graph[e].length;

		previous = v;
	    }

        }
    };

    DECLARE_TEMPUS_PLUGIN( ForbiddenMovementPlugin );
};



