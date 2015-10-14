#include <string>

#include "application.hh"
#include "plugin.hh"

#include "ch_query_graph.hh"

struct Edge
{
    Edge() {}
    Edge( uint32_t cost, bool is_shortcut ) : cost_(cost), is_shortcut_(is_shortcut) {}

    uint32_t cost_: 31;
    uint32_t is_shortcut_: 1;
};

int main( int argc, char *argv[] )
{
    using namespace Tempus;

    typedef CHQueryGraph<Edge> CHGraph;

    {
        //std::vector<uint32_t> target_nodes = { 1, 2,   1,
        //2,      2 };
        std::vector<std::pair<uint32_t, uint32_t>> vp = { { 0, 2 }, { 0, 3 }, { 0, 2 }, { 0, 3 },
                                                          { 1, 2 }, { 1, 3 }, { 1, 2 }, { 1, 3 } };
                                              
        std::vector<size_t> up_deg = { 2, 2, 0, 0 };
        //std::vector<size_t> dn_deg = { 1, 1, 0 };
        std::vector<Edge> costs = { { 2, false }, { 1, false }, { 3, false }, { 4, false }, { 5, false }, { 5, false }, { 5, false }, { 5, false } };

        CHGraph ch_graph( vp.begin(), 4, up_deg.begin(), costs.begin() );
        ch_graph.debug_print( std::cout );

        CHGraph::VertexIndex v = 0;
        for ( auto oeit = out_edges( v, ch_graph ).first; oeit != ch_graph.out_edges( v ).second; oeit++ ) {
            std::cout << "+" << " " << oeit->source() << "-" << oeit->target() << " " << oeit->property().cost_ << std::endl;
        }
        for ( auto oeit = in_edges( v, ch_graph ).first; oeit != ch_graph.in_edges( v ).second; oeit++ ) {
            std::cout << "-" << " " << oeit->source() << "-" << oeit->target() << " " << oeit->property().cost_ << std::endl;
        }
        
        for ( auto eit = edges( ch_graph ).first; eit != edges( ch_graph ).second; eit++ ) {
            std::cout << source( *eit, ch_graph ) << "->" << eit->target() << (eit->is_upward() ? "+" : "-") << std::endl;
        }
        std::cout << "oud: " << out_degree( v, ch_graph ) << " ind: " << in_degree( v, ch_graph ) << std::endl;
    }

    const std::string& dbstring = argc > 1 ? argv[1] : "dbname=tempus_test_db";

    Application* app = Application::instance();

    app->connect( dbstring );
    app->pre_build_graph();
    std::cout << "building the graph...\n";

    app->build_graph( /*consistency_check*/ false, "tempus" );

    PluginFactory::instance()->load( "ch_plugin" );

    std::auto_ptr<Plugin> plugin( PluginFactory::instance()->createPlugin( "ch_plugin" ) );

    const Multimodal::Graph& graph = *app->graph();
    const Road::Graph& road_graph = graph.road();

#if 0
    db_id_t origin_id = 10;
    db_id_t destination_id = 11;
#else
    db_id_t origin_id = 14333;
    db_id_t destination_id = 14803;
#endif

    Road::Vertex origin = 0, destination = 0;

    bool found_origin = false, found_destination = false;
    for ( auto vi = vertices( road_graph ).first; vi != vertices( road_graph ).second; vi++ ) {
        if ( road_graph[*vi].db_id() == origin_id ) {
            origin = *vi;
            found_origin = true;
            break;
        }
    }
    if ( !found_origin ) {
        std::cerr << "Cannot find orgin vertex ID " << origin_id << std::endl;
    }
    for ( auto vi = vertices( road_graph ).first; vi != vertices( road_graph ).second; vi++ ) {
        if ( road_graph[*vi].db_id() == destination_id ) {
            destination = *vi;
            found_destination = true;
            break;
        }
    }
    if ( !found_destination ) {
        std::cerr << "Cannot find destination vertex ID " << origin_id << std::endl;
    }

    std::cout << "origin " << origin << std::endl;
    std::cout << "destination " << destination << std::endl;

#if 1
    Request req;
    req.set_origin( origin );
    req.set_destination( destination );

    plugin->pre_process( req );
    plugin->process();
#endif
}
