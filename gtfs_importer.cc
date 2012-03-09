// --- unfinished ---

#include <stdio.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <vector>

#include "gtfs_importer.hh"
#include "multimodal_graph.hh"

namespace PT = Tempus::PublicTransport;

using namespace std;

namespace Tempus
{
    Tempus::Time parseTime( const std::string& str )
    {
	vector<string> fields;
	boost::split( fields, str,  boost::is_any_of(":") );
	Time t;
	int hours = boost::lexical_cast<int>( fields[0] );
	int mins = boost::lexical_cast<int>( fields[1] );
	int secs = boost::lexical_cast<int>( fields[2] );
	return hours * 60 * 60 + mins * 60 + secs;
    }

    class CSVReader
    {
    public:
	CSVReader( const std::string& file_name )
	{
	    fp = 0;
	    fp = fopen( file_name.c_str(), "r" );
	    if ( !fp )
	    {
		std::string err = "Error opening " + file_name;
		throw std::runtime_error( err );
	    }
	    lineno_ = 0;
	}

	bool get_next_line( std::vector<std::string>& rfields )
	{
	    if ( feof(fp) )
		return false;

	    fgets( line_, 1024, fp );
	    if ( lineno_ == 0 )
	    {
		// skip the first line
		fgets( line_, 1024, fp );
	    }
	    rfields.clear();
	    boost::split( rfields, line_,  boost::is_any_of(",") );
	    lineno_++;
	    return true;
	}

	virtual ~CSVReader()
	{
	    if ( fp )
		fclose( fp );
	}

    protected:
	FILE *fp;
	char line_[1024];
	size_t lineno_;
    };


    void GTFSImporter::import( const char* dir_name )
    {
	PT::Graph pt_graph;

	std::map<Tempus::db_id_t, PT::Stop> stops;
	std::map<Tempus::db_id_t, PT::Trip> trips;
	std::map<Tempus::db_id_t, PT::Route> routes;
	//
	// temporary object used to map string ID to integer ID
	map<string, Tempus::db_id_t> stop_id_map;
	map<string, Tempus::db_id_t> trip_id_map;
	map<string, Tempus::db_id_t> route_id_map;

	std::string dir = dir_name;
	std::vector<std::string> fields;

	CSVReader stops_reader( dir + "/stops.txt" );
	while ( stops_reader.get_next_line( fields ) )
	{
	    assert( fields.size() > 0 );
	    PT::Stop stop;
	    // process id
	    std::string str_id = fields[0];
	    stop.db_id = stop_id_map.size() + 1;
	    stop_id_map[ str_id ] = stop.db_id;
	    
	    /// trim quotes from name
	    stop.name = fields[1].substr( 1, fields[1].size() - 2 );
	    stop.is_station = boost::lexical_cast<int>(fields[7]);
	    
	    // TODO : process stops that have parents
	    std::string str_parent = fields[8];

	    PT::Vertex v = boost::add_vertex( stop, pt_graph );
	    
	    stops[ stop.db_id ] = stop;
	}

	CSVReader routes_reader( dir + "/routes.txt" );
	while ( routes_reader.get_next_line( fields ) )
	{
	    PT::Route route;
	    // process id
	    std::string str_id = fields[0];
	    route.db_id = route_id_map.size() + 1;
	    route_id_map[ str_id ] = route.db_id;

	    /// trim quotes from name
	    route.short_name = fields[2].substr( 1, fields[2].size() - 2 );
	    route.long_name = fields[3].substr( 1, fields[3].size() - 2 );

	    route.route_type = boost::lexical_cast<int>(fields[5]);

	    routes[ route.db_id ] = route;
	}

	CSVReader trips_reader( dir + "/trips.txt" );
	while ( trips_reader.get_next_line( fields ) )
	{
	    PT::Trip trip;
	    // process id
	    std::string str_id = fields[2];
	    trip.db_id = trip_id_map.size() + 1;
	    trip_id_map[ str_id ] = trip.db_id;

	    /// trim quotes from name
	    trip.short_name = fields[3].substr( 1, fields[2].size() - 2 );

	    std::string route_str_id = fields[0];
	    Tempus::db_id_t route_id = route_id_map[ route_str_id ];
	    if ( routes.find( route_id ) != routes.end() )
	    {
		routes[ route_id ].trips.push_back( trip );
	    }
	    else
	    {
		cerr << "Can't find route with ID " << route_str_id << endl;
	    }

	    trips[ trip.db_id ] = trip;
	}
    
	CSVReader stop_times_reader( dir + "/stop_times.txt" );
	while ( stop_times_reader.get_next_line( fields ) )
	{
	    PT::Trip::StopTime stop_time;
	    // process id
	    std::string trip_id_str = fields[0];
	    Tempus::db_id_t trip_id = trip_id_map[ trip_id_str ];

	    PT::Trip& trip = trips[ trip_id ];

	    std::string stop_id_str = fields[3];
	    Tempus::db_id_t stop_id = stop_id_map[ stop_id_str ];
	    stop_time.stop = &stops[ stop_id ];
	    stop_time.arrival_time = parseTime( fields[1] );
	    stop_time.departure_time = parseTime( fields[2] );

	    // FIXME : we assume stops are ordered

	    //TODO
	}
    }
};
