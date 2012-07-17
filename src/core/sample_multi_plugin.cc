#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"

using namespace std;

// TODO
// * include pt_transfer
// * POI ?
// * Filter pt networks

namespace Tempus
{
    class MultiPlugin : public Plugin
    {
    public:

	MultiPlugin( Db::Connection& db ) : Plugin( "multiplugin", db )
	{
	}

	virtual ~MultiPlugin()
	{
	}

    public:
	virtual void pre_process( Request& request ) throw (std::invalid_argument)
	{
	    request_ = request;
	    result_.clear();
 	}

	Multimodal::Vertex vertex_from_road_node_id( db_id_t id )
	{
	    Multimodal::VertexIterator vi, vi_end;
	    for ( boost::tie( vi, vi_end ) = vertices( graph_ ); vi != vi_end; vi++ )
	    {
		if ( vi->type == Multimodal::Vertex::Road && graph_.road[ vi->road_vertex ].db_id == id )
		{
		    return *vi;
		}
	    }
	}

	virtual void process()
	{
	    size_t n = num_vertices( graph_ );
	    std::vector<boost::default_color_type> color_map( n );
	    std::vector<Multimodal::Vertex> pred_map( n );
	    std::vector<double> distance_map( n );
	    std::map< Multimodal::Edge, double > lengths;

	    Multimodal::EdgeIterator ei, ei_end;
	    for ( boost::tie( ei, ei_end ) = edges( graph_ ); ei != ei_end; ei++ )
	    {
		if ( ei->connection_type() == Multimodal::Edge::Road2Road )
		{
		    lengths[ *ei ] = 10.0;
		}
		else
		{
		    lengths[ *ei ] = 1.0;
		}
	    }

	    Multimodal::Vertex origin( &graph_.road, request_.origin );
	    Multimodal::Vertex destination( &graph_.road, request_.destination() );

	    cout << "origin = " << origin << endl;
	    cout << "destination = " << destination << endl;

	    Multimodal::VertexIndexProperty vertex_index = get( boost::vertex_index, graph_ );

	    boost::dijkstra_shortest_paths( graph_,
	    				    origin,
	    				    boost::make_iterator_property_map( pred_map.begin(), vertex_index ),
	    				    boost::make_iterator_property_map( distance_map.begin(), vertex_index ),
	    				    boost::make_assoc_property_map( lengths ),
	    				    vertex_index,
	    				    std::less<double>(),
	    				    boost::closed_plus<double>(),
	    				    std::numeric_limits<double>::max(),
	    				    0.0,
	    				    boost::dijkstra_visitor<boost::null_visitor>(),
	    				    boost::make_iterator_property_map( color_map.begin(), vertex_index )
	    				    );
	    cout << "Dijkstra OK" << endl;

	    double d = distance_map[vertex_index[ destination ]];
	    if ( d < std::numeric_limits<double>::max() )
	    {
		cout << "Distance to " << destination << " = " << d << endl;
	    }

	    std::list<Multimodal::Vertex> path;
	    Multimodal::Vertex current = destination;
	    bool found = true;
	    while ( current != origin )
	    {
		path.push_front( current );
		Multimodal::Vertex ncurrent = pred_map[ vertex_index[ current ] ];
		if ( ncurrent == current )
		{
		    found = false;
		    break;
		}
		current = ncurrent;
	    }
	    if ( !found )
	    {
		cerr << "No path found" << endl;
		return;
	    }
	    path.push_front( origin );

	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();
	    std::list<Multimodal::Vertex>::const_iterator previous = path.begin();
	    std::cout << "first: " << *previous << std::endl;
	    std::list<Multimodal::Vertex>::const_iterator it = ++previous; --previous;
	    for ( ; it != path.end(); ++it )
	    {
		    if ( previous->type == Multimodal::Vertex::Road && it->type == Multimodal::Vertex::Road ) {
			    Roadmap::RoadStep* step = new Roadmap::RoadStep();
			    Road::Graph& road_graph = graph_.road;
			    Road::Edge e;
			    bool found = false;
			    boost::tie( e, found ) = edge( previous->road_vertex, it->road_vertex, *it->road_graph );
			    if ( !found ) {
				    std::cerr << "Can't find the road edge!!!" << std::endl;
				    continue;
			    }
			    step->road_section = e;
			    step->costs[CostDistance] += road_graph[e].length;
			    roadmap.total_costs[CostDistance] += road_graph[e].length;

			    // add to the roadmap
			    roadmap.steps.push_back( step );
		    }

		    else if ( previous->type == Multimodal::Vertex::PublicTransport && it->type == Multimodal::Vertex::PublicTransport ) {
			    Roadmap::PublicTransportStep* step = new Roadmap::PublicTransportStep();
			    PublicTransport::Edge e;
			    bool found = false;
			    boost::tie( e, found ) = edge( previous->pt_vertex, it->pt_vertex, *it->pt_graph );
			    if ( !found ) {
				    std::cerr << "Can't find the pt edge!!!" << std::endl;
				    continue;
			    }
			    step->section = e;
			    // Set the trip ID
			    step->trip_id = 1; 
			    // find the network_id
			    for ( Multimodal::Graph::PublicTransportGraphList::const_iterator nit = graph_.public_transports.begin(); nit != graph_.public_transports.end(); ++nit ) {
				    if ( it->pt_graph == &nit->second ) {
					    step->network_id = nit->first;
					    break;
				    }
			    }
			    // compute distance
			    step->costs[CostDistance] += 1.0;
			    roadmap.total_costs[CostDistance] += 1.0;

			    // add to the roadmap
			    roadmap.steps.push_back( step );
		    }

		    previous = it;
	    }
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

    };

    DECLARE_TEMPUS_PLUGIN( MultiPlugin );
};
