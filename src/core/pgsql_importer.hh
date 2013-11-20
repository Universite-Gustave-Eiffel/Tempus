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
    /// Import constants (road, transports types) into global variables.
    void import_constants( Multimodal::Graph& graph, ProgressionCallback& callback = null_progression_callback );

    ///
    /// Import the multimodal graph
    void import_graph( Multimodal::Graph& graph, ProgressionCallback& callback = null_progression_callback );

    ///
    /// Access to underlying connection object
    Db::Connection& get_connection() {
        return connection_;
    }

    ///
    /// Import turn restrictions
    Road::Restrictions import_turn_restrictions( const Road::Graph& graph );

protected:
    Db::Connection connection_;
};
} // Tempus namespace

#endif
