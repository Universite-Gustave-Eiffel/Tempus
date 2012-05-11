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
		if ( vi->is_road && graph_.road[ vi->road.vertex ].db_id == id )
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

	    Multimodal::Vertex origin, destination;
	    origin = vertex_from_road_node_id( 19953 );
	    destination = vertex_from_road_node_id( 22510 );

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
	    Multimodal::Vertex previous;
	    for ( std::list<Multimodal::Vertex>::const_iterator it = path.begin(); it != path.end(); it++ )
	    {
		roadmap.overview_path.push_back( coordinates( *it, db_, graph_ ) );
	    }

	    cout << "END" << endl;
	}

	Result& result()
	{
	    return result_;
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

    };

    DECLARE_TEMPUS_PLUGIN( MultiPlugin );
};
