// Plugin sample on a road graph
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT License

/**
   Sample plugin that processes very simple user request on a road graph.
   * Only distance minimization is considered
   * No intermediary point is supported

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

	RoadPlugin( Db::Connection& db ) : Plugin( "road_plugin", db )
	{
	    declare_option( "trace_vertex", "Trace vertex traversal", false );
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

	    get_option( "trace_vertex", trace_vertex_ );
	}

	virtual void road_vertex_accessor( Road::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
		if ( trace_vertex_ )
		{
		    // very slow
		    cout << "Examining vertex " << v << endl;
		}
	    }
	}

	virtual void process()
	{
	    Road::Vertex origin = request_.origin;
	    Road::Vertex destination = request_.destination();

	    Road::Graph& road_graph = graph_.road;

	    timeb t_start, t_stop;
	    ftime( &t_start );

	    std::vector<Road::Vertex> pred_map( boost::num_vertices(road_graph) );
	    std::vector<double> distance_map( boost::num_vertices(road_graph) );
	    ///
	    /// We define a property map that reads the 'length' (of type double) member of a Road::Section,
	    /// which is the edge property of a Road::Graph
	    FieldPropertyAccessor<Road::Graph, boost::edge_property_tag, double, double Road::Section::*> length_map( road_graph, &Road::Section::length );

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

	    ftime( &t_stop );
	    long long sstart = t_start.time * 1000L + t_start.millitm;
	    long long sstop = t_stop.time * 1000L + t_stop.millitm;
	    float time_s = float((sstop - sstart) / 1000.0);
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
		roadmap.overview_path.push_back( coordinates( v, db_, road_graph ) );
		
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
	    Tempus::db_id_t previous_section = 0;
	    bool on_roundabout = false;
	    bool was_on_roundabout = false;
	    // road section at the beginning of the roundabout
	    Tempus::db_id_t roundabout_enter;
	    // road section at the end of the roundabout
	    Tempus::db_id_t roundabout_leave;
	    Roadmap::RoadStep *last_step = 0;

	    Roadmap::RoadStep::EndMovement movement;
	    for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++ )
	    {
		Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *it );
		movement = Roadmap::RoadStep::GoAhead;

		on_roundabout =  road_graph[step->road_section].is_roundabout;

		bool action = false;
		if ( on_roundabout && !was_on_roundabout )
		{
		    // we enter a roundabout
		    roundabout_enter = road_graph[step->road_section].db_id;
		    movement = Roadmap::RoadStep::RoundAboutEnter;
		    action = true;
		}
		if ( !on_roundabout && was_on_roundabout )
		{
		    // we leave a roundabout
		    roundabout_leave = road_graph[step->road_section].db_id;
		    // FIXME : compute the exit number
		    movement = Roadmap::RoadStep::FirstExit;
		    action = true;
		}

		if ( previous_section && !on_roundabout && !action )
		{
		    string q1 = (boost::format("SELECT ST_Azimuth( endpoint(s1.geom), startpoint(s1.geom) ), ST_Azimuth( startpoint(s2.geom), endpoint(s2.geom) ), endpoint(s1.geom)=startpoint(s2.geom) "
					       "FROM tempus.road_section AS s1, tempus.road_section AS s2 WHERE s1.id=%1% AND s2.id=%2%") % previous_section % road_graph[step->road_section].db_id).str();
		    Db::Result res = db_.exec(q1);
		    double pi = 3.14159265;
		    double z1 = res[0][0].as<double>() / pi * 180.0;
		    double z2 = res[0][1].as<double>() / pi * 180.0;
		    bool continuous =  res[0][2].as<bool>();
		    if ( !continuous )
			z1 = z1 - 180;

		    int z = int(z1 - z2);
		    z = (z + 360) % 360;
		    if ( z >= 30 && z <= 150 )
		    {
			movement = Roadmap::RoadStep::TurnRight;
		    }
		    if ( z >= 210 && z < 330 )
		    {
			movement = Roadmap::RoadStep::TurnLeft;
		    }
		}

		road_name = road_graph[step->road_section].road_name;
		distance = road_graph[step->road_section].length;

		if ( last_step )
		{
		    last_step->end_movement = movement;
		    last_step->distance_km = -1.0;
		}

		switch ( movement )
		{
		case Roadmap::RoadStep::GoAhead:
		    cout << k++ << " - Walk on " << road_name << " for " << distance << "m" << endl;
		    break;
		case Roadmap::RoadStep::TurnLeft:
		    cout << k++ << " - Turn left on " << road_name << " and walk for " << distance << "m" << endl;
		    break;
		case Roadmap::RoadStep::TurnRight:
		    cout << k++ << " - Turn right on " << road_name << " and walk for " << distance << "m" << endl;
		    break;
		case Roadmap::RoadStep::RoundAboutEnter:
		    cout << k++ << " - Enter the roundabout on " << road_name << endl;
		    break;
		case Roadmap::RoadStep::FirstExit:
		case Roadmap::RoadStep::SecondExit:
		case Roadmap::RoadStep::ThirdExit:
		case Roadmap::RoadStep::FourthExit:
		case Roadmap::RoadStep::FifthExit:
		case Roadmap::RoadStep::SixthExit:
		    cout << k++ << " - Leave the roundabout on " << road_name << endl;
		    break;
		}
		previous_section = road_graph[step->road_section].db_id;
		was_on_roundabout = on_roundabout;
		last_step = step;
	    }
	    last_step->end_movement = Roadmap::RoadStep::YouAreArrived;
	    last_step->distance_km = -1.0;

	    return result_;
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

    };

    DECLARE_TEMPUS_PLUGIN( RoadPlugin );
};



