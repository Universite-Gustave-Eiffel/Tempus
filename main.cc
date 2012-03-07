#include "graph.hh"

int main()
{
    Tempus::Road::Graph road_graph;

    Tempus::Road::Vertex a = boost::add_vertex( road_graph );
    Tempus::Road::Vertex b = boost::add_vertex( road_graph );

    bool is_added;
    Tempus::Road::Edge e;
    tie( e, is_added ) = boost::add_edge( a, b, road_graph );
    road_graph[e].road_name = "Boulevard des martyrs Nantais de la Résistance";
    
    namespace PT = Tempus::PublicTransport;

    PT::Graph pt_graph;
    PT::Vertex m1 = boost::add_vertex( pt_graph );
    pt_graph[m1].name = "Vincent Gâche";
    pt_graph[m1].road_section = &road_graph[e];
    pt_graph[m1].abscissa_road_section = 0.1;
    pt_graph[m1].corresponding_vertex = m1;

    pt_graph[m1].check_consistency();

    PT::Vertex m2 = boost::add_vertex( pt_graph );
    pt_graph[m2].name = "Wattignies";
    pt_graph[m2].road_section = &road_graph[e];
    pt_graph[m2].abscissa_road_section = 0.5;

    PT::Vertex m3 = boost::add_vertex( pt_graph );
    pt_graph[m3].name = "Mangin";
    pt_graph[m3].road_section = &road_graph[e];
    pt_graph[m3].abscissa_road_section = 0.8;
    return 0;
}
