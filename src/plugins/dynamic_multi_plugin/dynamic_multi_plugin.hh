/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <boost/tuple/tuple_comparison.hpp>

#include "plugin.hh"
#include "automaton_lib/automaton.hh"
#include "cost_lib/cost_calculator.hh"

namespace Tempus {

// Data structure used to label vertices : (vertex, automaton state, mode)
struct Triple {
    Multimodal::Vertex vertex;
    Automaton<Road::Edge>::Graph::vertex_descriptor state;
    db_id_t mode;

    bool operator==( const Tempus::Triple& t ) const {
        return (vertex == t.vertex) && (state == t.state) && (mode == t.mode);
    }
    bool operator!=( const Tempus::Triple& t ) const {
        return (vertex != t.vertex) || (state != t.state) || (mode != t.mode);
    }
    bool operator<( const Triple& other ) const
    {
        // use tuple comparison operator (lexicographical search)
        return boost::tie( vertex,state,mode ) < boost::tie( other.vertex, other.state, other.mode );
    }
};

typedef std::list<Triple> Path;

typedef std::map< Triple, double > PotentialMap;
typedef std::map< Triple, db_id_t > TripMap;
typedef std::map< Triple, Triple > PredecessorMap;

// variables to retain between two requests
struct StaticVariables
{
    Date current_day; // Day for which timetable or frequency data are loaded
    TimetableMap timetable; // Timetable data for the current request
    FrequencyMap frequency; // Frequency data for the current request
    TimetableMap rtimetable; // Reverse time table
    FrequencyMap rfrequency; // Reverse frequency data for the current request

    RoadEdgeSpeedProfile speed_profile; // daily speed profile

    StaticVariables() : current_day( boost::gregorian::from_string("2013/11/12") )
    {}
};

class DynamicMultiPlugin : public Plugin
{
public:
    DynamicMultiPlugin( ProgressionCallback& progression, const VariantMap& options );

    static Plugin::OptionDescriptionList option_descriptions();
    static Plugin::Capabilities plugin_capabilities();

    const Automaton<Road::Edge>& automaton() const { return automaton_; }

    const Multimodal::Graph* graph() const { return graph_; }

public:
    virtual std::unique_ptr<PluginRequest> request( const PluginRequest::OptionValueList& options = PluginRequest::OptionValueList() ) const;

private:
    const Multimodal::Graph* graph_;
    Automaton<Road::Edge> automaton_;
};

class DynamicMultiPluginRequest : public PluginRequest
{
public:
    DynamicMultiPluginRequest( const DynamicMultiPlugin* plugin, const OptionValueList& options );

    virtual std::unique_ptr<Result> process( const Request& request );

private:
    Path reorder_path( Triple departure, Triple arrival, bool reverse = false );
    void add_roadmap( const Request& request, Result& r, const Path& path, bool reverse = false );

    static StaticVariables s_;

    PotentialMap potential_map_;
    PredecessorMap pred_map_;
    PotentialMap shift_map_;
    PotentialMap wait_map_;
    TripMap trip_map_;

    bool enable_trace_;

    const Multimodal::Graph* graph_;
};

}





