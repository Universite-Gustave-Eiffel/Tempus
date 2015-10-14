/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_PGSQL_IMPORTER_HH
#define TEMPUS_PGSQL_IMPORTER_HH

/**
   Tempus PostgreSQL importer.
 */

#include <string>

#include "multimodal_graph.hh"
#include "db.hh"

namespace Tempus {
class PQImporter {
public:
    PQImporter( const std::string& pg_options );
    virtual ~PQImporter() {}

    ///
    /// Query the database
    Db::Result query( const std::string& query_str );

    ///
    /// Import the multimodal graph
    std::unique_ptr<Multimodal::Graph> import_graph( ProgressionCallback& callback = null_progression_callback, bool consistency_check = true, const std::string& schema_name = "tempus" );

    ///
    /// Import constants transport modes
    void import_constants( Multimodal::Graph&, ProgressionCallback& callback = null_progression_callback, const std::string& schema_name = "tempus" );

    ///
    /// Import turn restrictions
    Road::Restrictions import_turn_restrictions( const Road::Graph& graph, const std::string& schema_name = "tempus" );

    ///
    /// Access to underlying connection object
    Db::Connection& get_connection() {
        return connection_;
    }

private:
    Db::Connection connection_;

    std::unique_ptr<Road::Graph> import_road_graph_( ProgressionCallback& callback,
                                                   bool consistency_check,
                                                   const std::string& schema_name,
                                                   std::map<Tempus::db_id_t, Road::Edge>&  );

};
} // Tempus namespace

#endif
