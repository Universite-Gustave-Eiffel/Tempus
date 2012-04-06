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

	RoadPlugin() : Plugin("road_plugin")
	{
	}

	virtual ~RoadPlugin()
	{
	}
    protected:
	std::string db_options_;

	///
	/// A CostMap maps publict transport edges to a double
	typedef std::map< Road::Edge, double > CostMap;

	// static cost map: length
	CostMap length_map_;
    public:
	virtual void pre_build( const std::string& options )
	{
	    db_options_ = options;
	}

	virtual void build()
	{
	    // request the database
	    PQImporter importer( db_options_ );
	    TextProgression progression(50);
	    std::cout << "Loading graph from database: " << std::endl;
	    cout << "Importing constants ... " << endl;
	    importer.import_constants();
	    cout << "Importing graph ... " << endl;
	    importer.import_graph( graph_, progression );

	    Road::EdgeIterator eb, ee;
	    Road::Graph& road_graph = graph_.road;
	    for ( boost::tie(eb, ee) = boost::edges( road_graph ); eb != ee; eb++ )
	    {
		length_map_[*eb] = road_graph[*eb].length;
	    }
	}

	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    REQUIRE( request.check_consistency() );
	    REQUIRE( request.steps.size() == 1 );

	    Road::Vertex origin = request.origin;
	    Road::Vertex destination = request.get_destination();
	    Road::Graph& road_graph = graph_.road;
	    REQUIRE( vertex_exists( origin, road_graph ) );
	    REQUIRE( vertex_exists( destination, road_graph ) );

	    if ( (request.optimizing_criteria[0] != CostDistance) )
	    {
		throw std::invalid_argument( "Unsupported optimizing criterion" );
	    }

	    request_ = request;
	}

	virtual void process()
	{
	    Road::Vertex origin = request_.origin;
	    Road::Vertex destination = request_.get_destination();

	    Road::Graph& road_graph = graph_.road;

	    std::vector<Road::Vertex> pred_map( boost::num_vertices(road_graph) );
	    std::vector<double> distance_map( boost::num_vertices(road_graph) );
	    boost::associative_property_map<CostMap> cost_property_map( length_map_ );

	    boost::dijkstra_shortest_paths( road_graph,
					    origin,
					    &pred_map[0],
					    &distance_map[0],
					    cost_property_map,
					    boost::get( boost::vertex_index, road_graph ),
					    std::less<double>(),
					    boost::closed_plus<double>(),
					    std::numeric_limits<double>::max(),
					    0.0,
					    boost::default_dijkstra_visitor()
					    );

	    // reorder the path, could have been better included ...
	    std::list<Road::Vertex> path;
	    Road::Vertex current = destination;
	    while ( current != origin )
	    {
		path.push_front( current );
		current = pred_map[ current ];
	    }
	    path.push_front( origin );

	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();
	    Roadmap::RoadStep* step = 0;
	    // FIXME: we add here another simple roadmap, made of RoadStep. Not very clean.
	    result_.push_back( Roadmap() );
	    Roadmap& simple_roadmap = result_.back();

	    Road::Edge current_road;
	    double distance = 0.0;
	    Road::Vertex previous;
	    bool first_loop = true;

	    for ( std::list<Road::Vertex>::iterator it = path.begin(); it != path.end(); it++ )
	    {
		Road::Vertex v = *it;
		// Simple roadmap
		Roadmap::VertexStep* road_step = new Roadmap::VertexStep();
		road_step->vertex = v;
		simple_roadmap.steps.push_back( road_step );
		
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
	    Roadmap& roadmap = result_.front();
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



