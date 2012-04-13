#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"

using std::cout;
using std::endl;

namespace Tempus
{
    class MyPlugin : public Plugin
    {
    public:

	MyPlugin( Db::Connection& db ) : Plugin( "myplugin", db )
	{
	}

	virtual ~MyPlugin()
	{
	}

    protected:
	///
	/// A CostMap maps publict transport edges to a double
	typedef std::map< PublicTransport::Edge, double > CostMap;

	// static cost map: length
	CostMap length_map_;
	CostMap duration_map_;
	// edge -> trip map
	std::map<PublicTransport::Edge, db_id_t> trip_id_map_;

	std::string db_options_;
    public:
	virtual void post_build()
	{
	    graph_ = Application::instance()->get_graph();

	    // Browse edges and compute their duration and length
	    // FIXME : not optimal. Would have to find a unique query for all the edges
	    // FIXME : duration is a dynamic value !
	    length_map_.clear();
	    duration_map_.clear();
	    trip_id_map_.clear();

	    PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
	    Road::Graph& road_graph = graph_.road;
	    PublicTransport::EdgeIterator eb, ee;
	    for ( tie( eb, ee ) = boost::edges( pt_graph ); eb != ee; eb++ )
	    {
		PublicTransport::Vertex s = boost::source( *eb, pt_graph );
		PublicTransport::Vertex t = boost::target( *eb, pt_graph );

		// Distance computation
		length_map_[ *eb ] = (1 - pt_graph[s].abscissa_road_section) * road_graph[ pt_graph[s].road_section ].length +
		    pt_graph[t].abscissa_road_section * road_graph[ pt_graph[t].road_section ].length;
		std::cout << pt_graph[s].name << " to " << pt_graph[t].name << " length = " << length_map_[ *eb ] << std::endl;

		// Duration computation
		db_id_t s_id = pt_graph[s].db_id;
		db_id_t t_id = pt_graph[t].db_id;
		std::string query = (boost::format( "select t1.departure_time, t2.departure_time - t1.arrival_time, t1.trip_id from tempus.pt_stop_time as t1, tempus.pt_stop_time as t2 "
						    "where t1.trip_id = t2.trip_id and t1.stop_id=%1% and t2.stop_id=%2% and t1.stop_sequence < t2.stop_sequence order by t1.departure_time" ) % s_id % t_id).str();
		Db::Result res = db_.exec( query );

		// FIXME : over simplification here: we always take the first row
		if ( res.size() >= 1 )
		{
		    Time duration = res[0][1].as<Time>();
		    int trip_id = res[0][2].as<int>();

		    BOOST_ASSERT( trip_id > 0 );
		    
		    std::cout << pt_graph[s].name << " to " << pt_graph[t].name << " duration = " << duration.n_secs << std::endl;
		    duration_map_[ *eb ] = duration.n_secs + .0;
		    trip_id_map_[ *eb ] = trip_id;
		}
	    }
	}

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
 	}

	virtual void process()
	{
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
		    node = request_.get_destination();
		
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
		    std::cout << "Road node #" << node << " <-> Public transport node '" << pt_graph[found_vertex].name << "'" << std::endl;
		}
	    }
	    
	    //
	    // Call to Dijkstra
	    //

	    std::vector<PublicTransport::Vertex> pred_map( boost::num_vertices(pt_graph) );
	    std::vector<double> distance_map( boost::num_vertices(pt_graph) );
	    
	    boost::associative_property_map<CostMap> cost_property_map;
	    if ( request_.optimizing_criteria[0] == CostDuration )
	    {
		cost_property_map = duration_map_;
	    }
	    if ( request_.optimizing_criteria[0] == CostDistance )
	    {
		cost_property_map = length_map_;
	    }

	    boost::dijkstra_shortest_paths( pt_graph,
					    departure,
					    &pred_map[0],
					    &distance_map[0],
					    cost_property_map,
					    boost::get( boost::vertex_index, pt_graph ),
					    std::less<double>(),
					    boost::closed_plus<double>(),
					    std::numeric_limits<double>::max(),
					    0.0,
					    boost::default_dijkstra_visitor()
					    );

	    // reorder the path, could have been better included ...
	    std::vector<PublicTransport::Vertex> path;
	    PublicTransport::Vertex current = arrival;
	    while ( current != departure )
	    {
		path.push_back( current );
		current = pred_map[ current ];
	    }
	    path.push_back( departure );

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
	    PublicTransport::Vertex previous;
	    bool first_loop = true;
	    for ( int i = path.size() - 1; i >= 0; i-- )
	    {
		PublicTransport::Vertex v = path[i];
		if ( first_loop )
		{
		    previous = v;
		    first_loop = false;
		    continue;
		}

		// Find an edge, based on a source and destination vertex (boost::edge)
		PublicTransport::Edge e;
		bool found = false;
		boost::tie( e, found ) = boost::edge( previous, v, pt_graph );
		BOOST_ASSERT( found );

		// if the trip_id changes, we create another step
		BOOST_ASSERT( trip_id_map_[ e ] != 0 );
		if ( current_trip != trip_id_map_[ e ] )
		{
		    current_trip = trip_id_map_[ e ];
		    step = new Roadmap::PublicTransportStep();
		    roadmap.steps.push_back( step );
		    step->departure_stop = previous;
		    step->trip_id = current_trip;
		}
		double duration = duration_map_[ e ];
		double length = length_map_[ e ];

		if ( step->costs.find( CostDuration ) == step->costs.end() )
		    step->costs[ CostDuration ] = 0.0;
		step->costs[ CostDuration ] += duration;
		if ( step->costs.find( CostDistance ) == step->costs.end() )
		    step->costs[ CostDistance ] = 0.0;
		step->costs[ CostDistance ] += length;
		// simplification
		step->arrival_stop = v;
		roadmap.total_costs[ CostDuration ] += duration;
		roadmap.total_costs[ CostDistance ] += length;
		
		previous = v;
	    }
	}

	Result& result()
	{
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

    DECLARE_TEMPUS_PLUGIN( MyPlugin );
};



