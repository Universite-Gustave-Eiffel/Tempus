#include <boost/graph/depth_first_search.hpp>
#include <iostream>

#include "road_graph.hh"
#include "public_transport_graph.hh"
#include "plugin.hh"
#include "pgsql_importer.hh"

using namespace std;

namespace PT = Tempus::PublicTransport;

struct MyVisitor
{
    void initialize_vertex( PT::Vertex s, PT::Graph g )
    {
    }
    void start_vertex( PT::Vertex s, PT::Graph g )
    {
    }
    void discover_vertex( PT::Vertex s, PT::Graph g )
    {
	cout << "Discovering " << g[s].name << endl;
    };
    void finish_vertex( PT::Vertex s, PT::Graph g )
    {
    };
    
    void examine_edge( PT::Edge e, PT::Graph )
    {
    }
    void tree_edge( PT::Edge e, PT::Graph )
    {
    }
    void back_edge( PT::Edge e, PT::Graph )
    {
    }
    void forward_or_cross_edge( PT::Edge e, PT::Graph )
    {
    }
};

struct TextProgression : public Tempus::ProgressionCallback
{
public:
    TextProgression( int N = 50 ) : N_(N)
    {
    }
    virtual void operator()( float percent, bool finished )
    {
	std::cout << "\r";
	int n = percent * N_;
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

int main()
{
    Tempus::Road::Graph road_graph;

    Tempus::Road::Vertex a = boost::add_vertex( road_graph );
    Tempus::Road::Vertex b = boost::add_vertex( road_graph );

    bool is_added;
    Tempus::Road::Edge e;
    tie( e, is_added ) = boost::add_edge( a, b, road_graph );
    road_graph[e].road_name = "Boulevard des martyrs Nantais de la Résistance";

    road_graph[e].graph = &road_graph;
    road_graph[e].edge = e;

    PT::Network tan;
    tan.name = "TAN";

    PT::Graph pt_graph;
    PT::Vertex m1 = boost::add_vertex( pt_graph );
    PT::Stop& stop = pt_graph[m1];
    stop.name = "Vincent Gâche";
    stop.road_section = &road_graph[e];
    stop.abscissa_road_section = 0.1;

    BOOST_ASSERT( stop.check_consistency() );

    PT::Stop stop2;
    stop2.name = "Wattignies";
    stop2.road_section = &road_graph[e];
    stop2.abscissa_road_section = 0.5;
    // explicit use of an additional parameter to model vertex properties
    PT::Vertex m2 = boost::add_vertex( stop2, pt_graph );

    PT::Vertex m3 = boost::add_vertex( pt_graph );
    pt_graph[m3].name = "Mangin";
    pt_graph[m3].road_section = &road_graph[e];
    pt_graph[m3].abscissa_road_section = 0.8;


    PT::Trip trip;
    trip.short_name = "Vers Mangin";

    PT::Edge e1, e2;
    tie( e1, is_added) = boost::add_edge( m1, m2, pt_graph );
    tie( e2, is_added) = boost::add_edge( m2, m3, pt_graph );

    PT::Calendar cal;
    trip.service = &cal;

    BOOST_ASSERT( trip.check_consistency() );

    PT::Route route;
    route.network = &tan;
    route.short_name = "Mangin";
    route.long_name = "Vers Mangin en Tram";
    route.route_type = PT::Route::TypeTram;
    route.trips.push_back( trip );

    BOOST_ASSERT( route.check_consistency() );

    //
    // Exterior properties (a.k.a. dynamic valuations)
    //
    MyVisitor visitor;    

    // Associative array (i.e. std::map)
    typedef std::map< PT::Vertex, boost::default_color_type > AssociativePTColorMap;
    AssociativePTColorMap colors;
    boost::associative_property_map<AssociativePTColorMap> colors_property_map( colors );
    boost::depth_first_search( pt_graph, visitor, colors_property_map );

    ///
    /// Plugins

    void* h = Tempus::Plugin::load( "dummy_plugin" );

    for (std::list<Tempus::Plugin*>::iterator it = Tempus::Plugin::plugins.begin(); it != Tempus::Plugin::plugins.end(); it++ )
    {
	std::cout << "plugin " << (*it)->get_name() << std::endl;
    }

    if ( h )
    {
	Tempus::Plugin::unload( h );
    }

    Tempus::PQImporter importer( "dbname = tempus" );
    
    Tempus::MultimodalGraph graph;
    TextProgression progression(50);
    std::cout << "Loading graph from database: " << std::endl;
    importer.import_graph( graph, progression );

    return 0;
}
