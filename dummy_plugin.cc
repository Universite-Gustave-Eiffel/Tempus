#include "plugin.hh"

#include "pgsql_importer.hh"
#include <pqxx/pqxx>
#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

//
// FIXME : to be replaced by a config file
#define DB_CONNECTION_OPTIONS "dbname=tempus"

namespace Tempus
{
    class MyPlugin : public Plugin
    {
    public:

	struct TextProgression : public Tempus::ProgressionCallback
	{
	public:
	    TextProgression( int N = 50 ) : N_(N)
	    {
	    }
	    virtual void operator()( float percent, bool finished )
	    {
		std::cout << "\r";
		int n = int(percent * N_);
		std::cout << "[";
		for (int i = 0; i < n; i++)
		    std::cout << ".";
		for (int i = n; i < N_; i++)
		    std::cout << " ";
		std::cout << "] ";
		std::cout << int(percent * 100 ) << "%";
		if ( finished )
		    std::cout << std::endl;
		std::cout.flush();
	    }
	protected:
	    int N_;
	};

	MyPlugin() : Plugin("myplugin")
	{
	}

	virtual ~MyPlugin()
	{
	}

	virtual void build()
	{
	    // request the database
		PQImporter importer( DB_CONNECTION_OPTIONS );
	    TextProgression progression(50);
	    std::cout << "Loading graph from database: " << std::endl;
	    importer.import_graph( graph_, progression );
	}

	virtual void process( Request& request )
	{
	    REQUIRE( request.check_consistency() );
	    REQUIRE( request.steps.size() == 1 );

	    request_ = request;
	    if ( request.optimizing_criteria[0] != CostDuration )
	    {
		throw std::runtime_error( "Unsupported optimizing criterion" );
	    }
	    PublicTransport::Graph pt_graph = graph_.public_transports.front();
	    Road::Graph& road_graph = graph_.road;

	    PublicTransport::Vertex departure, arrival;
	    // for each step in the request, find the corresponding public transport node
	    for ( size_t i = 0; i < 2; i++ )
	    {
		Road::Vertex node;
		if ( i == 0 )
		    node = request.origin;
		else
		    node = request.get_destination();
		
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

	    // minimize duration
	    // FIXME include db access in PQImporter
	    PQImporter importer( DB_CONNECTION_OPTIONS );
	    pqxx::work w( importer.get_connection() );
	    
	    // map used by dijkstra
	    typedef std::map< PublicTransport::Edge, double > DistanceMap;
	    DistanceMap duration_map;
	    std::map<PublicTransport::Edge, db_id_t> trip_id_map;
	    
	    PublicTransport::EdgeIterator eb, ee;
	    // FIXME : not optimal. Would have to find a unique query for all the edges
	    for ( tie( eb, ee ) = boost::edges( pt_graph ); eb != ee; eb++ )
	    {
		PublicTransport::Vertex s = boost::source( *eb, pt_graph );
		PublicTransport::Vertex t = boost::target( *eb, pt_graph );
		int s_id = pt_graph[s].db_id;
		int t_id = pt_graph[t].db_id;
		std::string query = (boost::format( "select t1.departure_time, t2.departure_time - t1.arrival_time, t1.trip_id from tempus.pt_stop_time as t1, tempus.pt_stop_time as t2 "
						    "where t1.trip_id = t2.trip_id and t1.stop_id=%1% and t2.stop_id=%2% and t1.stop_sequence < t2.stop_sequence order by t1.departure_time" ) % s_id % t_id).str();
		pqxx::result res = w.exec( query );

		// over simplification here: we always take the first row
		if ( res.size() >= 1 )
		{
		    Time duration = res[0][1].as<Time>();
		    int trip_id = res[0][2].as<int>();
		    
		    std::cout << pt_graph[s].name << " to " << pt_graph[t].name << " duration = " << duration.n_secs << std::endl;
		    duration_map[ *eb ] = duration.n_secs + .0;
		    trip_id_map[ *eb ] = trip_id;
		}
	    }
	
	    // Dijkstra

	    boost::associative_property_map<DistanceMap> duration_property_map( duration_map );

	    std::vector<PublicTransport::Vertex> pred_map( boost::num_vertices(pt_graph) );
	    std::vector<double> distance_map( boost::num_vertices(pt_graph) );
	    
	    boost::dijkstra_shortest_paths( pt_graph,
					    departure,
					    &pred_map[0],
					    &distance_map[0],
					    duration_property_map,
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
	    Roadmap::Step* step = 0;
	    roadmap.total_costs[ CostDuration ] = 0.0;

	    //
	    // for each step in the graph, find the common trip and add each step to the roadmap
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

		// we look for the corresponding edge, based on a source and destination vertex
		// FIXME: there may be other way !
		boost::graph_traits<PublicTransport::Graph>::out_edge_iterator edge_it_b, edge_it_e;
		for ( boost::tie(edge_it_b, edge_it_e) = boost::out_edges( previous, pt_graph ); edge_it_b != edge_it_e; edge_it_b++ )
		{
		    if ( boost::target(*edge_it_b, pt_graph) == v )
		    {
			// if the trip_id changes, we create another step
			if ( current_trip != trip_id_map[ *edge_it_b ] )
			{
			    current_trip = trip_id_map[ *edge_it_b ];
			    roadmap.steps.push_back( Roadmap::Step() );
			    step = &roadmap.steps.back();
			    step->pt.departure_stop = previous;
			    step->pt.trip_id = current_trip;
			}
			double duration = duration_map[ *edge_it_b ];
			step->costs[ CostDuration ] += duration;
			// simplification
			step->transport_type = TransportTramway;
			step->pt.arrival_stop = v;
			roadmap.total_costs[ CostDuration ] += duration;

			break;
		    }
		}
		previous = v;
	    }
	}

	void result()
	{
	    Roadmap& roadmap = result_.back();
	    PublicTransport::Graph& pt_graph = graph_.public_transports.back();

	    std::cout << "Total duration: " << roadmap.total_costs[CostDuration] << std::endl;
	    std::cout << "Number of changes: " << (roadmap.steps.size() - 1) << std::endl;
	    int k = 1;
	    for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++, k++ )
	    {
		std::cout << k << " - Take the trip #" << it->pt.trip_id << " from '" << pt_graph[it->pt.departure_stop].name << "' to '" << pt_graph[it->pt.arrival_stop].name << "'" << std::endl;
		std::cout << "Duration: " << it->costs[CostDuration] << std::endl << std::endl;
	    }
	}

    };

    DECLARE_TEMPUS_PLUGIN( MyPlugin );
};



