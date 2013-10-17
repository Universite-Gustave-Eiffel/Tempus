// (c) 2012 Oslandia
// MIT License

#ifndef TEMPUS_APPLICATION_HH
#define TEMPUS_APPLICATION_HH

#include <string>

#include "multimodal_graph.hh"
#include "db.hh"

namespace Tempus
{
    struct PluginFactory;

    ///
    /// Class used to represent the global state of the current application
    class Application
    {
    public:
	///
	/// Access to the singleton instance
	static Application* instance();

	///
	/// Used to represent the application state
	enum State
	{
	    ///
	    /// The application has just been (re)started
	    Started = 0,
	    ///
	    /// The application has database connection informations
	    Connected,
	    ///
	    /// Graph has been pre built
	    GraphPreBuilt,
	    ///
	    /// Graph has been built
	    GraphBuilt
	};
	
	///
	/// State accessors
	State state() const { return state_; }

	///
	/// Connect to the database
	/// @param[in] db_options string giving options for database connection (e.g. dbname="" user="", etc.)
	void connect( const std::string& db_options );

        ///
        /// Get the directory where data are stored
        const std::string data_directory() const;

	///
	/// Database connection accessors. @relates Db::Connection
	const std::string& db_options() const { return db_options_; }

	///
	/// Method to call to pre build the graph in memory
	void pre_build_graph();

	///
	/// Build the graph in memory (import from the database and wake up plugins)
	void build_graph();

	///
	/// Graph accessor (non const)
	Multimodal::Graph& graph() { return graph_; }

    protected:
	// private constructor
	Application() 
        {}

	std::string db_options_;
	Multimodal::Graph graph_;
	
	State state_;
    };
}

#ifdef _WIN32
///
/// The global instance accessor must be callable as a C function under Windows
extern "C" __declspec(dllexport) Tempus::Application* get_application_instance_();
#endif

#endif
