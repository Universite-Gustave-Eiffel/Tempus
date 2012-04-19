// Plugin sample on a road graph
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

/**
   Sample plugin that processes very simple user request on a road graph.
   * Only distance minimization is considered
   * No intermediary point is supported

   The plugin finds a route between an origin and a destination via Dijkstra.
 */

#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;

namespace Tempus
{
    class RoadPlugin : public Plugin
    {
    public:

	RoadPlugin( Db::Connection& db ) : Plugin( "road_plugin", db )
	{
	}

	virtual ~RoadPlugin()
	{
	}
    public:
	virtual void post_build()
	{
	    graph_ = Application::instance()->graph();
	    Road::EdgeIterator eb, ee;
	    Road::Graph& road_graph = graph_.road;
	}

	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    REQUIRE( request.check_consistency() );
	    REQUIRE( request.steps.size() == 1 );

	    Road::Vertex origin = request.origin;
	    Road::Vertex destination = request.destination();
	    Road::Graph& road_graph = graph_.road;
	    REQUIRE( vertex_exists( origin, road_graph ) );
	    REQUIRE( vertex_exists( destination, road_graph ) );

	    if ( (request.optimizing_criteria[0] != CostDistance) )
	    {
		throw std::invalid_argument( "Unsupported optimizing criterion" );
	    }

	    request_ = request;
	}

	virtual void road_vertex_accessor( Road::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
		// very slow
		// cout << "Examining vertex " << v << endl;
	    }
	}

	virtual void process()
	{
	    Road::Vertex origin = request_.origin;
	    Road::Vertex destination = request_.destination();

	    Road::Graph& road_graph = graph_.road;

	    timeval tv_start, tv_stop;
	    gettimeofday( &tv_start, NULL );

	    std::vector<Road::Vertex> pred_map( boost::num_vertices(road_graph) );
	    std::vector<double> distance_map( boost::num_vertices(road_graph) );
	    ///
	    /// We define a property map that reads the 'length' (of type double) member of a Road::Section,
	    /// which is the edge property of a Road::Graph
	    FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, &Road::Section::length> length_map( road_graph );

	    ///
	    /// Visitor to be built on 'this'. This way, xxx_accessor methods will be called
	    Tempus::PluginRoadGraphVisitor vis( this );

	    boost::dijkstra_shortest_paths( road_graph,
					    origin,
					    &pred_map[0],
					    &distance_map[0],
					    length_map,
					    boost::get( boost::vertex_index, road_graph ),
					    std::less<double>(),
					    boost::closed_plus<double>(),
					    std::numeric_limits<double>::max(),
					    0.0,
					    vis
					    );

	    gettimeofday( &tv_stop, NULL );
	    long long sstart = tv_start.tv_sec * 1000000LL + tv_start.tv_usec;
	    long long sstop = tv_stop.tv_sec * 1000000LL + tv_stop.tv_usec;
	    float time_s = (sstop - sstart) / 1000000.0;
	    metrics_[ "time_s" ] = time_s;

	    // reorder the path, could have been better included ...
	    std::list<Road::Vertex> path;
	    Road::Vertex current = destination;
	    bool found = true;
	    while ( current != origin )
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
	    path.push_front( origin );

	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();
	    Roadmap::RoadStep* step = 0;

	    Road::Edge current_road;
	    double distance = 0.0;
	    Road::Vertex previous;
	    bool first_loop = true;

	    for ( std::list<Road::Vertex>::iterator it = path.begin(); it != path.end(); it++ )
	    {
		Road::Vertex v = *it;
		// Overview path
		roadmap.overview_path.push_back( v );
		
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

		if ( step == 0 || e != current_road )
		{
		    step = new Roadmap::RoadStep();
		    roadmap.steps.push_back( step );
		    step->road_section = e;
		    current_road = e;
		}
		step->costs[CostDistance] += road_graph[e].length;
		roadmap.total_costs[CostDistance] += road_graph[e].length;
		
		previous = v;
	    }
	}

	Result& result()
	{
	    Roadmap& roadmap = result_.back();
	    Road::Graph& road_graph = graph_.road;
	    
	    std::cout << "Total distance: " << roadmap.total_costs[CostDistance] << std::endl;
	    int k = 1;
	    string road_name = "";
	    double distance = 0.0;
	    for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++ )
	    {
		Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *it );
		string l_road_name = road_graph[step->road_section].road_name;
		if ( road_name != "" && l_road_name != road_name )
		{
		    cout << k << " - Walk on " << road_name << " for " << distance << "m" << endl;
		    k++;
		    road_name = l_road_name;
		    distance = 0.0;
		}
		if ( road_name == "" )
		    road_name = l_road_name;
		distance += step->costs[CostDistance];
	    }

	    return result_;
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

    };

    DECLARE_TEMPUS_PLUGIN( RoadPlugin );
};



