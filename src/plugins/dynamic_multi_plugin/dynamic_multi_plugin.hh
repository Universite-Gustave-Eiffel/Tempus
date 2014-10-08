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
#include "automaton_lib/cost_calculator.hh"

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
    Automaton<Road::Edge> automaton;
    TimetableMap timetable; // Timetable data for the current request
    FrequencyMap frequency; // Frequency data for the current request
    TimetableMap rtimetable; // Reverse time table

    StaticVariables() : current_day( boost::gregorian::from_string("2013/11/12") )
    {}
};

class DynamicMultiPlugin : public Plugin {
public:

    static const OptionDescriptionList option_descriptions();
    static const PluginParameters plugin_parameters();

    DynamicMultiPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~DynamicMultiPlugin() {
    }
    
protected:
    // Plugin options
    double timetable_frequency_; // travel time calculation mode
    bool verbose_algo_; // verbose vertex and edge traversal 
    bool verbose_; // Verbose processing (except algorithm)
    double min_transfer_time_; // Minimum time necessary for a transfer to be done (in minutes) 
    double walking_speed_; // Average walking speed
    double cycling_speed_; // Average cycling speed 
    double car_parking_search_time_; // Parking search time for cars
    
    // Plugin metrics
    int iterations_; // Number of iterations
    double time_; // Time elapsed for pre_process() and process() 
    double time_algo_; // Time elapsed for process()
    
    // Other attributes
    Multimodal::Vertex destination_; // Current request destination 
    std::map< Multimodal::Vertex, db_id_t > available_vehicles_; 
    PotentialMap potential_map_;     
    PredecessorMap pred_map_; 
    PotentialMap wait_map_; 
    TripMap trip_map_; 

    Road::Vertex parking_location_;

    static StaticVariables s_;
	
public: 
    static void post_build();
    virtual void pre_process( Request& request );
    virtual void process();
private:
    Path reorder_path( Triple departure, Triple arrival, bool reverse = false );
    void add_roadmap( const Path& path, bool reverse = false );
}; 

}





