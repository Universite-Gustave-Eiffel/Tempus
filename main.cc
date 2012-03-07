#include "road_graph.hh"
#include "public_transport_graph.hh"

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

    namespace PT = Tempus::PublicTransport;

    PT::Network tan;
    tan.name = "TAN";

    PT::Graph pt_graph;
    PT::Vertex m1 = boost::add_vertex( pt_graph );
    PT::Stop& stop = pt_graph[m1];
    stop.name = "Vincent Gâche";
    stop.road_section = &road_graph[e];
    stop.abscissa_road_section = 0.1;

    BOOST_ASSERT( stop.check_consistency() );

    PT::Vertex m2 = boost::add_vertex( pt_graph );
    pt_graph[m2].name = "Wattignies";
    pt_graph[m2].road_section = &road_graph[e];
    pt_graph[m2].abscissa_road_section = 0.5;

    PT::Vertex m3 = boost::add_vertex( pt_graph );
    pt_graph[m3].name = "Mangin";
    pt_graph[m3].road_section = &road_graph[e];
    pt_graph[m3].abscissa_road_section = 0.8;


    PT::Trip trip;
    trip.short_name = "Vers Mangin";
    trip.stop_sequence.push_back( &pt_graph[m1] );
    trip.stop_sequence.push_back( &pt_graph[m2] );
    trip.stop_sequence.push_back( &pt_graph[m3] );

    BOOST_ASSERT( trip.check_consistency() );

    PT::Route route;
    route.network = &tan;
    route.short_name = "Mangin";
    route.long_name = "Vers Mangin en Tram";
    route.route_type = PT::Route::TypeTram;
    route.trips.push_back( trip );

    BOOST_ASSERT( route.check_consistency() );

    return 0;
}
