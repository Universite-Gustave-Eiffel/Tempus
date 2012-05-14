#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"

using namespace std;

namespace Tempus
{

    class LengthCalculator
    {
    public:
	LengthCalculator( Db::Connection& db ) : db_(db) {}

	// TODO - possible improvement: cache length
	double operator()( PublicTransport::Graph& graph, PublicTransport::Edge& e )
	{
	    Db::Result res = db_.exec( (boost::format("SELECT ST_Length(geom) FROM tempus.pt_section WHERE stop_from = %1% AND stop_to = %2%")
				       % graph[e].stop_from % graph[e].stop_to ).str() );
	    BOOST_ASSERT( res.size() > 0 );
	    double l = res[0][0].as<double>();
	    return l;
	}
    protected:
	Db::Connection& db_;
    };

    class PtPlugin : public Plugin
    {
    public:

	PtPlugin( Db::Connection& db ) : Plugin( "myplugin", db )
	{
	}

	virtual ~PtPlugin()
	{
	}

    public:
	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    REQUIRE( graph_.public_transports.size() >= 1 );
	    REQUIRE( request.check_consistency() );
	    REQUIRE( request.steps.size() == 1 );

	    if ( (request.optimizing_criteria[0] != CostDuration) && (request.optimizing_criteria[0] != CostDistance) )
	    {
		throw std::invalid_argument( "Unsupported optimizing criterion" );
	    }
	    
	    request_ = request;
	    result_.clear();
 	}

	virtual void pt_edge_accessor( PublicTransport::Edge e, int access_type )
	{
	    // if ( access_type == Plugin::ExamineAccess )
	    // {
	    // 	PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
	    // 	cout << "Examining " << pt_graph[e].db_id << endl;
	    // }
	}
	virtual void pt_vertex_accessor( PublicTransport::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
	     	PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
	     	cout << "Examining vertex " << pt_graph[v].db_id << endl;
	    }
	}
	virtual void process()
	{
	    cout << "origin = " << request_.origin << " dest = " << request_.destination() << endl;
	    PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
	    Road::Graph& road_graph = graph_.road;

	    PublicTransport::Vertex departure, arrival;
	    // for each step in the request, find the corresponding public transport node
	    for ( size_t i = 0; i < 2; i++ )
	    {
		Road::Vertex node;
		if ( i == 0 )
		    node = request_.origin;
		else
		    node = request_.destination();
		
		bool found = false;
		PublicTransport::Vertex found_vertex;

		PublicTransport::VertexIterator vb, ve;
		for ( boost::tie(vb, ve) = boost::vertices( pt_graph ); vb != ve; vb++ )
		{
		    PublicTransport::Stop& stop = pt_graph[*vb];
		    Road::Edge section = stop.road_section;
		    Road::Vertex s, t;
		    s = boost::source( section, road_graph );
		    t = boost::target( section, road_graph );
		    if ( (node == s) || (node == t) )
		    {
			found_vertex = *vb;
			found = true;
			break;
		    }
		}
		
		if (found)
		{
		    if ( i == 0 )
			departure = found_vertex;
		    if ( i == 1 )
			arrival = found_vertex;
		    std::cout << "Road node #" << node << " <-> Public transport node " << pt_graph[found_vertex].db_id << std::endl;
		}
		else
		{
		    cerr << "Cannot find road node" << endl;
		    return;
		}
	    }
	    cout << "departure = " << departure << " arrival = " << arrival << endl;
	    
	    //
	    // Call to Dijkstra
	    //

	    std::vector<PublicTransport::Vertex> pred_map( boost::num_vertices(pt_graph) );
	    std::vector<double> distance_map( boost::num_vertices(pt_graph) );

	    LengthCalculator length_calculator( db_ );
	    FunctionPropertyAccessor<PublicTransport::Graph,
				     boost::edge_property_tag,
				     double,
				     LengthCalculator> length_map( pt_graph, length_calculator );
	    
	    PluginPtGraphVisitor vis( this );
	    boost::dijkstra_shortest_paths( pt_graph,
					    departure,
					    &pred_map[0],
					    &distance_map[0],
					    length_map,
					    boost::get( boost::vertex_index, pt_graph ),
					    std::less<double>(),
					    boost::closed_plus<double>(),
					    std::numeric_limits<double>::max(),
					    0.0,
					    vis
					    );

	    // reorder the path
	    std::list<PublicTransport::Vertex> path;
	    PublicTransport::Vertex current = arrival;
	    bool found = true;
	    while ( current != departure )
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
		cerr << "No path found" << endl;
		return;
	    }
	    path.push_front( departure );

	    //
	    // Final result building.
	    //

	    // we result in only one roadmap
	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();
	    Roadmap::PublicTransportStep* step = 0;
	    roadmap.total_costs[ CostDuration ] = 0.0;
	    roadmap.total_costs[ CostDistance ] = 0.0;

	    //
	    // for each step in the graph, find the common trip and add each step to the roadmap

	    // The current trip is set to 0, which means 'null'. This holds because every db's id are 1-based
	    db_id_t current_trip = 0;
	    bool first_loop = true;
		
	    step = new Roadmap::PublicTransportStep();
	    roadmap.steps.push_back( step );
	    // default network
	    step->network_id = 1;
	    step->departure_stop = *path.begin();
	    std::list<PublicTransport::Vertex>::iterator it = path.end();
	    it--;
	    step->arrival_stop = *it;

	    for ( it = path.begin(); it != path.end(); it++ )
	    {
		Point2D p = coordinates(*it, db_, pt_graph );
		roadmap.overview_path.push_back( p );
	    }

	    step->costs[ CostDistance ] = distance_map[ step->arrival_stop ];

	    roadmap.total_costs[ CostDistance ] += step->costs[ CostDistance ];
	}

	Result& result()
	{
	    if (result_.size() == 0)
		return result_;

	    Roadmap& roadmap = result_.back();
	    PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;

	    std::cout << "Total duration: " << roadmap.total_costs[CostDuration] << std::endl;
	    std::cout << "Total distance: " << roadmap.total_costs[CostDistance] << std::endl;
	    std::cout << "Number of changes: " << (roadmap.steps.size() - 1) << std::endl;
	    int k = 1;
	    for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++, k++ )
	    {
		Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( *it );
		std::cout << k << " - Take the trip #" << step->trip_id << " from '" << pt_graph[step->departure_stop].name << "' to '" << pt_graph[step->arrival_stop].name << "'" << std::endl;
		std::cout << "Duration: " << step->costs[CostDuration] << "s" << std::endl;
		std::cout << "Distance: " << step->costs[CostDistance] << "km" << std::endl;
		std::cout << std::endl;
	    }
	    return result_;
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

    };

    DECLARE_TEMPUS_PLUGIN( PtPlugin );
};



