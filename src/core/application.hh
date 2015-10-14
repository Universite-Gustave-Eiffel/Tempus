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

#ifndef TEMPUS_APPLICATION_HH
#define TEMPUS_APPLICATION_HH

#include <string>

#include <boost/optional.hpp>

#include "multimodal_graph.hh"
#include "db.hh"
#include "variant.hh"

namespace Tempus {
struct PluginFactory;

///
/// Class used to represent the global state of the current application
class Application {
public:
    ///
    /// Access to the singleton instance
    static Application* instance();

    ~Application();

    ///
    /// Used to represent the application state
    enum State {
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
    State state() const {
        return state_;
    }

    ///
    /// Connect to the database
    /// @param[in] db_options string giving options for database connection (e.g. dbname="" user="", etc.)
    void connect( const std::string& db_options );

    ///
    /// Get the directory where data are stored
    const std::string data_directory() const;

    ///
    /// Database connection accessors. @relates Db::Connection
    const std::string& db_options() const {
        return db_options_;
    }

    ///
    /// Method to call to pre build the graph in memory
    void pre_build_graph();


    void set_option( const std::string& key, const Variant& value );
    Variant option( const std::string& ) const;

    ///
    /// Build the graph in memory (import from the database and wake up plugins)
    void build_graph( bool consistency_check = false, const std::string& schema_name = "tempus" );

    ///
    /// Get the current schema used
    std::string schema_name() const;

    ///
    /// Graph accessor (non const)
    boost::optional<const Multimodal::Graph&> graph() {
        if (!graph_.get()) {
            return boost::optional<const Multimodal::Graph&>();
        }
        return *graph_;
    }

protected:
    // private constructor
    Application();

    std::string db_options_;
    std::string schema_name_;
    std::unique_ptr<Multimodal::Graph> graph_;

    State state_;

    std::map<std::string, Variant> options_;
};
}

#ifdef _WIN32
///
/// The global instance accessor must be callable as a C function under Windows
extern "C" __declspec( dllexport ) Tempus::Application* get_application_instance_();
#endif

#endif
