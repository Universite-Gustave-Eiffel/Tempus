// Plugin sample on a road graph
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

/**
   Sample plugin that processes very simple user request on a road graph.
   * Only distance minimization is considered

   The plugin finds a route between an origin and a destination via Dijkstra.
 */

#include <sys/timeb.h>
#include <time.h>

#ifdef _WIN32
#define ftime(x) _ftime(x)
#define timeb _timeb
#endif
 
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
        static const OptionDescriptionList option_descriptions()
        { 
            OptionDescriptionList odl;
	    odl.declare_option( "trace_vertex", "Trace vertex traversal", false );
            return odl; 
        }

	RoadPlugin( const std::string& name, Db::Connection& db ) : Plugin( name, db )
	{
	}

	virtual ~RoadPlugin()
	{
	}
    protected:
	bool trace_vertex_;
    public:
	virtual void post_build()
	{
	    graph_ = Application::instance()->graph();
	}

	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    REQUIRE( request.check_consistency() );

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

	    get_option( "trace_vertex", trace_vertex_ );
            COUT << "trace_vertex: " << trace_vertex_ << std::endl;

	    result_.clear();
	}

	virtual void road_vertex_accessor( Road::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
		if ( trace_vertex_ )
		{
		    // very slow
		    COUT << "Examining vertex " << v << endl;
		}
	    }
	}

	virtual void process()
	{
	    Road::Graph& road_graph = graph_.road;

	    timeb t_start, t_stop;
	    ftime( &t_start );

	    std::vector<Road::Vertex> pred_map( boost::num_vertices(road_graph) );
	    std::vector<double> distance_map( boost::num_vertices(road_graph) );
	    ///
	    /// We define a property map that reads the 'length' (of type double) member of a Road::Section,
	    /// which is the edge property of a Road::Graph
            //	    FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, double Road::Section::*> length_map( road_graph, &Road::Section::length );
            boost::property_map<Road::Graph, double Road::Section::*>::type length_map( get(&Road::Section::length, road_graph ) );

	    ///
	    /// Visitor to be built on 'this'. This way, xxx_accessor methods will be called
	    Tempus::PluginRoadGraphVisitor vis( this );

	    std::list<Road::Vertex> path;
	    Road::Vertex origin = request_.origin;
	    // resolve each step, in reverse order
	    for ( size_t ik = request_.steps.size(); ik >= 1; --ik ) {
		    size_t i = ik - 1;
		    if ( i > 0 )
			    origin = request_.steps[i-1].destination;
		    else
			    origin = request_.origin;
		    Road::Vertex destination = request_.steps[i].destination;

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
		    
		    // reorder the path, could have been better included ...
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
			   CERR << "No path found !" << endl;
			    return;
		    }
	    }
	    path.push_front( origin );

	    ftime( &t_stop );
	    long long sstart = t_start.time * 1000L + t_start.millitm;
	    long long sstop = t_stop.time * 1000L + t_stop.millitm;
	    float time_s = float((sstop - sstart) / 1000.0);
	    metrics_[ "time_s" ] = time_s;	    

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

	void cleanup()
	{
	    // nothing special to clean up
	}
    };
}
DECLARE_TEMPUS_PLUGIN( "sample_road_plugin", Tempus::RoadPlugin );



