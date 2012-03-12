// Tempus PostgreSQL importer
// (c) 2012 Oslandia
// MIT License

/**
   Tempus PostgreSQL importer. Uses libpqxx
 */

#ifndef TEMPUS_PGSQL_IMPORTER_HH
#define TEMPUS_PGSQL_IMPORTER_HH

#include <pqxx/connection>

#include <string>

#include "multimodal_graph.hh"

namespace Tempus
{
    class ProgressionCallback
    {
    public:
	virtual void operator()( float percent, bool finished = false )
	{
	    // Default : do nothing
	}
    };

    extern ProgressionCallback null_progression_callback;

    class PQImporter
    {
    public:
	PQImporter( const std::string& pg_options );
	virtual ~PQImporter() {}

	void import_graph( MultimodalGraph& graph, ProgressionCallback& callback = null_progression_callback ); 

    protected:
	pqxx::connection connection_;
    };
}; // Tempus namespace

#endif
