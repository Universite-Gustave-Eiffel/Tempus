#include <boost/format.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "plugin.hh"
#include "pgsql_importer.hh"
#include "db.hh"
#include "utils/graph_db_link.hh"
#include "utils/function_property_accessor.hh"

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
        
        static const OptionDescriptionList option_descriptions()
        {
            OptionDescriptionList odl;
            odl.declare_option( "origin_pt_stop", "Origin stop node", 0 );
            odl.declare_option( "destination_pt_stop", "Destination stop node", 0 );
            return odl; 
        }

	PtPlugin( const std::string & nname, const std::string & db_options ) : Plugin( nname, db_options )
	{
	}

	virtual ~PtPlugin()
	{
	}

    public:
	virtual void pre_process( Request& request )
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

	virtual void pt_vertex_accessor( PublicTransport::Vertex v, int access_type )
	{
	    if ( access_type == Plugin::ExamineAccess )
	    {
	     	PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
                CERR << "Examining vertex " << pt_graph[v].db_id << endl;
	    }
	}
	virtual void process()
	{
	    PublicTransport::Graph& pt_graph = graph_.public_transports.begin()->second;
            db_id_t network_id = graph_.public_transports.begin()->first;

	    PublicTransport::Vertex departure, arrival;

            // if stops are given by the corresponding options, get them
            int departure_id, arrival_id;
	    get_option( "origin_pt_stop", departure_id );
	    get_option( "destination_pt_stop", arrival_id );
            if ( departure_id != 0 && arrival_id != 0 ) {
                CERR << "departure id " << departure_id << " arrival id " << arrival_id << std::endl;
                bool found_v;
                boost::tie(departure,found_v) = vertex_from_id( departure_id, pt_graph );
                if ( ! found_v ) {
                    throw std::runtime_error( "Cannot find departure ID" );
                }
                boost::tie(arrival,found_v) = vertex_from_id( arrival_id, pt_graph );
                if ( ! found_v ) {
                    throw std::runtime_error( "Cannot find arrival ID" );
                }
            }
            else {
                // else use regular road nodes from the request
                Road::Graph& road_graph = graph_.road;
                CERR << "origin = " << road_graph[request_.origin].db_id << " dest = " << road_graph[request_.destination()].db_id << endl;

                // for each step in the request, find the corresponding public transport node
                for ( size_t i = 0; i < 2; i++ )
                {
                    Road::Vertex node;
                    if ( i == 0 )
                        node = request_.origin;
                    else
                        node = request_.destination();

                    PublicTransport::Vertex found_vertex;

                    std::string q = (boost::format("select s.id from tempus.road_node as n join tempus.pt_stop as s on st_dwithin( n.geom, s.geom, 100 ) "
                                                   "where n.id = %1% order by st_distance( n.geom, s.geom) asc limit 1") % road_graph[node].db_id ).str();
                    Db::Result res = db_.exec(q);
                    if ( res.size() < 1 ) {
                        throw std::runtime_error( (boost::format("Cannot find node %1%") % node).str() );
                    }
                    db_id_t vid = res[0][0].as<db_id_t>();
                    found_vertex = vertex_from_id( vid, pt_graph ).first;
                    {
                        if ( i == 0 )
                            departure = found_vertex;
                        if ( i == 1 )
                            arrival = found_vertex;
                        CERR << "Road node #" << node << " <-> Public transport node " << pt_graph[found_vertex].db_id << std::endl;
                    }
                }
            }
            CERR << "departure = " << departure << " arrival = " << arrival << endl;
	    
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
            {
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
                    throw std::runtime_error( "No path found" );
                }
                path.push_front( departure );
            }

	    //
	    // Final result building.
	    //

	    // we result in only one roadmap
	    result_.push_back( Roadmap() );
	    Roadmap& roadmap = result_.back();
	    Roadmap::PublicTransportStep* step = 0;
	    roadmap.total_costs[ CostDuration ] = 0.0;
	    roadmap.total_costs[ CostDistance ] = 0.0;

	    bool first_loop = true;

	    Road::Vertex previous = *path.begin();
	    for ( std::list<PublicTransport::Vertex>::iterator it = path.begin(); it != path.end(); it++ )
            {
                CERR << "path " << pt_graph[ *it ].db_id << std::endl;
                if ( first_loop ) {
                    first_loop = false;
                    continue;
                }
                step = new Roadmap::PublicTransportStep();
                roadmap.steps.push_back( step );

                bool found_e = false;
                PublicTransport::Edge e;
                boost::tie( e, found_e ) = boost::edge( previous, *it, pt_graph );

                step->section = e;
                // default
                step->network_id = network_id;
                step->trip_id = 1;

                previous = *it;

                step->costs[ CostDistance ] = distance_map[ *it ];
                roadmap.total_costs[ CostDistance ] += step->costs[ CostDistance ];
            }
	}

	void cleanup()
	{
	    // nothing special to clean up
	}

        static void post_build(){}

    };
}
DECLARE_TEMPUS_PLUGIN( "sample_pt_plugin", Tempus::PtPlugin )



