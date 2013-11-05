// Tempus PostgreSQL importer
// (c) 2012 Oslandia
// MIT License

#ifndef TEMPUS_PGSQL_IMPORTER_HH
#define TEMPUS_PGSQL_IMPORTER_HH

/**
   Tempus PostgreSQL importer.
 */

#include <string>

#include "multimodal_graph.hh"
#include "db.hh"

namespace Tempus
{
    class PQImporter
    {
    public:
	PQImporter( const std::string& pg_options );
	virtual ~PQImporter() {}

	///
	/// Query the database
	Db::Result query( const std::string& query_str );

	///
	/// Import constants (road, transports types) into global variables.
	void import_constants( Multimodal::Graph& graph, ProgressionCallback& callback = null_progression_callback );

	///
	/// Import the multimodal graph
	void import_graph( Multimodal::Graph& graph, ProgressionCallback& callback = null_progression_callback ); 

	///
	/// Access to underlying connection object
	Db::Connection& get_connection() { return connection_; }

        ///
        /// Import turn restrictions
        Road::Restrictions import_turn_restrictions( const Road::Graph& graph );

    protected:
	Db::Connection connection_;
    };
} // Tempus namespace

#endif
