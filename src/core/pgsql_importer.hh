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
    class PQImporter
    {
    public:
	PQImporter( const std::string& pg_options );
	virtual ~PQImporter() {}

	///
	/// Query the database
	pqxx::result query( const std::string& query_str );

	///
	/// Import constants (road, transports types) into global variables.
	void import_constants( ProgressionCallback& callback = null_progression_callback );

	///
	/// Import the multimodal graph
	void import_graph( MultimodalGraph& graph, ProgressionCallback& callback = null_progression_callback ); 

	///
	/// Access to underlying connection object
	pqxx::connection& get_connection() { return connection_; }

    protected:
	pqxx::connection connection_;
    };
}; // Tempus namespace

namespace pqxx
{
    ///
    /// Specialization of the from_string<> template used inside pqxx (in result[i].as<>() for instance)
    template<>
    inline void from_string<Tempus::Time>(const char str[],
					  Tempus::Time &time)
    {
	int h, m, s;
	sscanf( str, "%d:%d:%d", &h, &m, &s );
	time.n_secs = s + m * 60 + h * 3600;
    }

    template<>
    struct string_traits<Tempus::Time>
    {
	static const char *name() { return "Time"; }
	static bool has_null() { return true; }
	static bool is_null(const Tempus::Time& t) { return t.n_secs == -1; }
	static Tempus::Time null()
	{ Tempus::Time t; t.n_secs = -1; return t;}
	//	static void from_string(const char Str[], Tempus::Time& Obj) { }
	//	static Tempus::Time to_string(const Tempus::Time& Obj) { }
    };

};

#endif
