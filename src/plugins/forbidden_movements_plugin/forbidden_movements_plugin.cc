// Plugin sample on a road graph
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

#include <boost/format.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>

#include "utils/associative_property_map_default_value.hh"

#include "plugin.hh"
#include "road_graph.hh"
#include "utils/graph_db_link.hh"
#include "utils/field_property_accessor.hh"

#include "combined_graph.hh"
#include "combined_dijkstra.hh"

using namespace std;

namespace Tempus
{
    //
    // definition of the forbidden movements automaton
    
    struct AutomatonNode
    {
        double penalty; // penalty associated with the current state
    };
    
    struct AutomatonEdge
    {
        Road::Vertex source, target;
        // source and target vertex in the graph
    };
    
    // forbidden movement automata construction
    typedef boost::adjacency_list< boost::vecS, boost::vecS,
                                   boost::directedS,
                                   AutomatonNode,
                                   AutomatonEdge> Automaton;

    static Automaton automaton;

    class ForbiddenMovementPlugin : public Plugin
    {
    public:

        static const OptionDescriptionList option_descriptions()
        { 
            OptionDescriptionList odl;
            odl.declare_option("approach", "Use combined Dijkstra (0) or the combined graph adaptor (1)", 1 );
            return odl; 
        }

	ForbiddenMovementPlugin( const std::string& nname, const std::string & db_options ) : Plugin( nname, db_options )
	{
            OptionDescriptionList odl(option_descriptions());
            odl.set_options_default_value(this);
	}

	virtual ~ForbiddenMovementPlugin()
	{
	}


    private:

        Road::Vertex source_, destination_;

    public:

	static void post_build()
	{
            Road::Graph& road_graph = Application::instance()->graph().road;

            Automaton::vertex_descriptor v1, v2, v3;
            Automaton::edge_descriptor e1, e2;
            v1 = boost::add_vertex( automaton );
            automaton[ v1 ].penalty = 0.0;
            v2 = boost::add_vertex( automaton );
            automaton[ v2 ].penalty = 0.0;
            v3 = boost::add_vertex( automaton );
            automaton[ v3 ].penalty = 100000.0;

            bool ok;
            boost::tie( e1, ok ) = boost::add_edge( v1, v2, automaton );
            REQUIRE( ok );
            boost::tie( e2, ok ) = boost::add_edge( v2, v3, automaton );
            REQUIRE( ok );

            // forbidden edges (on tempus_test_db)
            automaton[ e1 ].source = vertex_from_id( 21556, road_graph );
            automaton[ e1 ].target = vertex_from_id( 21652, road_graph );

            automaton[ e2 ].source = vertex_from_id( 21652, road_graph );
            automaton[ e2 ].target = vertex_from_id( 21617, road_graph );
	}

	virtual void pre_process( Request& /* request */ ) 
	{
            // request is ignored for now
	    result_.clear();

            // source node fixed to 21687 (db's id)
            // target node : 21422

            Road::Graph& road_graph = graph_.road;
            source_ = vertex_from_id( 21687, road_graph );
            destination_ = vertex_from_id( 21422, road_graph );

            COUT << "source: " << source_ << std::endl;
            COUT << "destination: " << destination_ << std::endl;
	}

	virtual void process()
	{
            // copy the automaton
	    Road::Graph& road_graph = graph_.road;

            // The Label type which is a pair of (graph vertex, automaton state)
            typedef std::pair< Road::Vertex, boost::graph_traits<Automaton>::vertex_descriptor > Label;

            // predecessor map
            typedef std::map<Label, Label> PredecessorMap;
            PredecessorMap predecessor_map;
            // adaptation to a property map
            boost::associative_property_map< PredecessorMap > predecessor_pmap( predecessor_map );

            // distance map
            std::map<Label, double> distance_map;
            // adaptation to a property map
            associative_property_map_default_value< std::map<Label, double> > distance_pmap( distance_map, std::numeric_limits<double>::max() );

            // the weight is handled by the Road::Section::length field
	    typedef FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, double Road::Section::*> WeightMap;
            //            boost::property_map< Road::Graph, double Road::Section::*>::type weight_map ( boost::get( &Road::Section::length, road_graph ) );
            WeightMap weight_map( road_graph, &Road::Section::length );

            // the penalty of a state is handled by the AutomatonNode::penalty property
	    typedef FieldPropertyAccessor<Automaton, boost::vertex_property_tag, double, double AutomatonNode::*> PenaltyMap;
            PenaltyMap penalty_map( automaton, &AutomatonNode::penalty );

            Road::Vertex start_vertex = source_;
            Road::Vertex target_vertex = destination_;

            int approach;
            get_option( "approach", approach );
            if ( approach == 0 ) {
                combined_dijkstra( road_graph,
                                   automaton,
                                   penalty_map,
                                   start_vertex,
                                   target_vertex,
                                   predecessor_pmap,
                                   distance_pmap,
                                   weight_map );
            }
            else if ( approach == 1 ) {
                
                // the combined graph type
                typedef CombinedGraphAdaptor<Road::Graph, Automaton> CGraph;
                CGraph cgraph( road_graph, automaton );
               
                // the combined weight map made of a weight map on the road graph and a penalty map
                typedef CombinedWeightMap< CGraph, WeightMap, PenaltyMap > CombinedWeightMap;
                CombinedWeightMap wmap( cgraph, weight_map, penalty_map );
                
                // manually set the distance of the start_vertex to 0.0 (no init here)
                Automaton::vertex_descriptor q0 = *( vertices( automaton ).first );
                put( distance_pmap, std::make_pair( start_vertex, q0 ), 0.0 );
                
                // we call the _no_init variant here,
                // otherwise the distance_map (and predecessor_map) would be initialized on a potentially big
                // number of vertices
                boost::dijkstra_shortest_paths_no_color_map_no_init( cgraph,
                                                                     std::make_pair(start_vertex, q0),
                                                                     predecessor_pmap,
                                                                     distance_pmap,
                                                                     wmap,
                                                                     get( boost::vertex_index, cgraph ),
                                                                     std::less<double>(),
                                                                     boost::closed_plus<double>(),
                                                                     std::numeric_limits<double>::max(),
                                                                     0.0,
                                                                     boost::dijkstra_visitor<boost::null_visitor>()
                                                                     );
            }

            std::list<Road::Vertex> path;
            Label current = std::make_pair( target_vertex, 0 );
            while ( current.first != start_vertex )
            {
                path.push_front( current.first );
                current = predecessor_map[ current ];
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

}
DECLARE_TEMPUS_PLUGIN( "forbidden_movements", Tempus::ForbiddenMovementPlugin )



