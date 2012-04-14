#ifndef TEMPUS_APPLICATION_HH
#define TEMPUS_APPLICATION_HH

#include <string>

#include "db.hh"
#include "multimodal_graph.hh"
#include "plugin.hh"

namespace Tempus
{
    class Plugin;

    class Application
    {
    protected:
	// private constructor
	Application() {}

	Db::Connection db_;
	std::string db_options_;
	MultimodalGraph graph_;

    public:
	static Application* instance();

	Db::Connection& db_connection() { return db_; }
	void connect( const std::string& db_options );

	Plugin* load_plugin( const std::string& name );
	void unload_plugin( Plugin* plugin );

	void pre_build_graph();
	void build_graph();

	Tempus::MultimodalGraph& get_graph() { return graph_; }
    };
};

#endif
