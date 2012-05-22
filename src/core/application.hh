// (c) 2012 Oslandia
// MIT License

#ifndef TEMPUS_APPLICATION_HH
#define TEMPUS_APPLICATION_HH

#include <string>

#include "multimodal_graph.hh"
#include "plugin.hh"
#include "db.hh"

namespace Tempus
{
    class Plugin;

    class Application
    {
    public:
	static Application* instance();

	///
	/// Used to represent the application state
	enum State
	{
	    /// The application has just been (re)started
	    Started = 0,
	    /// The application is connected to a database
	    Connected,
	    /// Graph has been pre built
	    GraphPreBuilt,
	    /// Graph has been built
	    GraphBuilt
	};
	
	void state( State state ) { state_ = state; }
	State state() const { return state_; }

	void connect( const std::string& db_options );
	Db::Connection& db_connection() { return db_; }
	const std::string& db_options() const { return db_options_; }

	Plugin* load_plugin( const std::string& name );
	void unload_plugin( Plugin* plugin );

	void pre_build_graph();
	void build_graph();

	Multimodal::Graph& graph() { return graph_; }
    protected:
	// private constructor
	Application() {}

	Db::Connection db_;
	std::string db_options_;
	Multimodal::Graph graph_;
	
	State state_;
    };
};

#ifdef _WIN32
extern "C" __declspec(dllexport) Tempus::Application* get_application_instance_();
#endif

#endif
