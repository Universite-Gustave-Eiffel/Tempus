#include <string>
#include <iostream>
#include <memory>

#include <boost/format.hpp>

#include "plugin.hh"


namespace Tempus
{

    PluginFactory::~PluginFactory()
    {
        for ( DllMap::iterator i=dll_.begin(); i!=dll_.end(); i++)
        {
#ifdef _WIN32
            FreeLibrary( i->second.first );
#else
            if ( dlclose( i->second.first ) )
            {
                std::cerr << "Error on dlclose " << dlerror() << std::endl;
            }
#endif
        }
    }

    void PluginFactory::load( const std::string& dll_name )
    {
        if ( dll_.find(dll_name) != dll_.end() ) return;// already there 
        const std::string complete_dll_name = DLL_PREFIX + dll_name + DLL_SUFFIX;
        std::cout << "Loading " << complete_dll_name << std::endl;
#ifdef _WIN32
        HMODULE h = LoadLibrary( complete_dll_name.c_str() );
        if ( h == NULL )
        {
            throw std::runtime_error("DLL loading problem: "+std::string(GetLastError()));
        }
        PluginCreationFct createFct = (PluginCreationFct) GetProcAddress( h, "createPlugin" );
        if ( createFct == NULL )
        {
            throw std::runtime_error("GetProcAddress problem: "+std::string(GetLastError()));
            FreeLibrary( h );
        }
#else
        HMODULE h = dlopen( complete_dll_name.c_str(), RTLD_NOW | RTLD_GLOBAL );
        if ( !h )
        {
            throw std::runtime_error( dlerror() );
        }
        PluginCreationFct createFct = (PluginCreationFct)dlsym( h, "createPlugin" );
        if ( createFct == 0 )
        {
            dlclose( h );
            throw std::runtime_error( dlerror() );
        }
#endif
        dll_.insert( std::make_pair( dll_name, std::make_pair( h, createFct ) ) );
    }

    std::vector<std::string> PluginFactory::plugin_list() const
    {
        std::vector<std::string> names;
        for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++)
            names.push_back( i->first );
        return names;
    }

    Plugin * PluginFactory::createPlugin( const std::string & dll_name )
    {
        load( dll_name );
        DllMap::const_iterator dll = dll_.find(dll_name);
        std::auto_ptr<Plugin> p( dll->second.second( Application::instance()->db_connection() ) );
        p->post_build();
        p->validate();
        return p.release();
    }
    

    Plugin::Plugin( const std::string& name, Db::Connection& db ) :
	name_(name),
	db_(db),
	graph_(Application::instance()->graph())
    {
	// default metrics
	metrics_[ "time_s" ] = (float)0.0;
	metrics_[ "iterations" ] = (int)0;
    }

    void Plugin::set_option_from_string( const std::string& name, const std::string& value)
    {
	if ( options_descriptions_.find( name ) == options_descriptions_.end() )
	    return;
	OptionType t = options_descriptions_[name].type;
	switch (t)
	{
	case BoolOption:
	    options_[name] = boost::lexical_cast<int>( value ) == 0 ? false : true;
	    break;
	case IntOption:
	    options_[name] = boost::lexical_cast<int>( value );
	    break;
	case FloatOption:
	    options_[name] = boost::lexical_cast<float>( value );
	    break;
	case StringOption:
	    options_[name] = value;
	    break;
	default:
	    throw std::runtime_error( "Unknown type" );
	}
    }

    std::string Plugin::option_to_string( const std::string& name )
    {
	if ( options_descriptions_.find( name ) == options_descriptions_.end() )
	{
	    throw std::invalid_argument( "Cannot find option " + name );
	}
	OptionType t = options_descriptions_[name].type;
	boost::any value = options_[name];
	if ( value.empty() )
	    return "";

	switch (t)
	{
	case BoolOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<bool>(value) ? 1 : 0 );
	case IntOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<int>(value) );
	case FloatOption:
	    return boost::lexical_cast<std::string>( boost::any_cast<float>(value) );
	case StringOption:
	    return boost::any_cast<std::string>(value);
	default:
	    throw std::runtime_error( "Unknown type" );
	}
	// never happens
	return "";
    }

    std::string Plugin::metric_to_string( const std::string& name )
    {
	if ( metrics_.find( name ) == metrics_.end() )
	{
	    throw std::invalid_argument( "Cannot find metric " + name );
	}
	boost::any v = metrics_[name];
	if ( v.empty() )
	    return "";

	if ( v.type() == typeid( bool ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<bool>( v ) ? 1 : 0 );
	}
	else if ( v.type() == typeid( int ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<int>( v ) );
	}
	else if ( v.type() == typeid( float ) )
	{
	    return boost::lexical_cast<std::string>( boost::any_cast<float>( v ) );
	}
	else if ( v.type() == typeid( std::string ) )
	{
	    return boost::any_cast<std::string>( v );
	}
	throw std::invalid_argument( "No known conversion for metric " + name );
	// never happens
	return "";
    }

    void Plugin::post_build()
    {
	std::cout << "[plugin_base]: post_build" << std::endl;
    }

    void Plugin::validate()
    {
	std::cout << "[plugin_base]: validate" << std::endl;
    }

    void Plugin::cycle()
    {
	std::cout << "[plugin_base]: cycle" << std::endl;
    }

    void Plugin::pre_process( Request& request ) throw (std::invalid_argument)
    {
	std::cout << "[plugin_base]: pre_process" << std::endl;
	request_ = request;
	result_.clear();
    }

    ///
    /// Process the user request.
    /// Must populates the 'result_' object.
    void Plugin::process()
    {
	std::cout << "[plugin_base]: process" << std::endl;
    }

    void Plugin::post_process()
    {
	std::cout << "[plugin_base]: post_process" << std::endl;
    }

    ///
    /// Text formatting and preparation of roadmap
    Result& Plugin::result()
    {
	for ( Result::iterator rit = result_.begin(); rit != result_.end(); ++rit ) {
	    Roadmap& roadmap = *rit;
	    Road::Graph& road_graph = graph_.road;
	    
	    // display the global costs of the result
	    for ( Costs::const_iterator it = roadmap.total_costs.begin(); it != roadmap.total_costs.end(); ++it ) {
		std::cout << "Total " << cost_name( it->first ) << ": " << it->second << cost_unit( it->first ) << std::endl;
	    }
	    
	    std::string road_name = "";
	    double distance = 0.0;
	    Tempus::db_id_t previous_section = 0;
	    bool on_roundabout = false;
	    bool was_on_roundabout = false;
	    // road section at the beginning of the roundabout
	    Tempus::db_id_t roundabout_enter;
	    // road section at the end of the roundabout
	    Tempus::db_id_t roundabout_leave;
	    Roadmap::RoadStep *last_step = 0;
	    
	    // Counter for direction texts
	    int direction_i = 1;
	    Roadmap::RoadStep::EndMovement movement;
	    for ( Roadmap::StepList::iterator it = roadmap.steps.begin(); it != roadmap.steps.end(); it++ )
	    {
		if ( (*it)->step_type == Roadmap::Step::GenericStep ) {
		    Roadmap::GenericStep* step = static_cast<Roadmap::GenericStep*>( *it );
		    Multimodal::Edge* edge = static_cast<Multimodal::Edge*>( step );

		    bool is_road_pt = false;
		    db_id_t road_id, pt_id;
		    switch ( edge->connection_type()) {
		    case Multimodal::Edge::Road2Transport: {
			is_road_pt = true;
			const Road::Graph& road_graph = *(edge->source.road_graph);
			const PublicTransport::Graph& pt_graph = *(edge->target.pt_graph);
			road_id = road_graph[ edge->source.road_vertex ].db_id;
			pt_id = pt_graph[ edge->target.pt_vertex ].db_id;

			std::cout << direction_i++ << " - Go to the station " << pt_graph[ edge->target.pt_vertex ].name << std::endl;

		    } break;
		    case Multimodal::Edge::Transport2Road: {
			is_road_pt = true;
			const PublicTransport::Graph& pt_graph = *(edge->source.pt_graph);
			const Road::Graph& road_graph = *(edge->target.road_graph);
			pt_id = pt_graph[ edge->source.pt_vertex ].db_id;
			road_id = road_graph[ edge->target.road_vertex ].db_id;				 

			std::cout << direction_i++ << " - Leave the station " << pt_graph[ edge->source.pt_vertex ].name << std::endl;

		    } break;
		    }

		    if ( is_road_pt ) {
			std::string query = (boost::format( "SELECT st_asbinary(st_force2d(st_makeline(t1.geom, t2.geom))) from "
							    "(select geom from tempus.road_node where id=%1%) as t1, "
							    "(select geom from tempus.pt_stop where id=%2%) as t2" ) %
					     road_id %
					     pt_id ).str();
			Db::Result res = db_.exec(query);
			BOOST_ASSERT( res.size() > 0 );
			std::string wkb = res[0][0].as<std::string>();
			// get rid of the heading '\x'
			step->geometry_wkb = wkb.substr(2);
		    }
		}
		else if ( (*it)->step_type == Roadmap::Step::PublicTransportStep ) {
		    Roadmap::PublicTransportStep* step = static_cast<Roadmap::PublicTransportStep*>( *it );
		    PublicTransport::Graph& pt_graph = graph_.public_transports[step->network_id];
		    
		    //
		    // retrieval of the step's geometry
		    std::string q = (boost::format("SELECT st_asbinary(st_force2d(geom)) FROM tempus.pt_section WHERE stop_from=%1% AND stop_to=%2%") %
				     pt_graph[step->section].stop_from % pt_graph[step->section].stop_to ).str();
		    Db::Result res = db_.exec(q);
		    std::string wkb = res[0][0].as<std::string>();
		    // get rid of the heading '\x'
		    step->geometry_wkb = wkb.substr(2);
		    
		    PublicTransport::Vertex v1 = vertex_from_id( pt_graph[step->section].stop_from, pt_graph );
		    PublicTransport::Vertex v2 = vertex_from_id( pt_graph[step->section].stop_to, pt_graph );
		    std::cout << direction_i++ << " - Take the trip #" << step->trip_id << " from '" << pt_graph[v1].name << "' to '" << pt_graph[v2].name << "' (";
		    // display associated costs
		    for ( Costs::const_iterator cit = step->costs.begin(); cit != step->costs.end(); ++cit ) {
			std::cout << cost_name( cit->first ) << ": " << cit->second << cost_unit( cit->first ) << " ";
		    }
		    std::cout << ")" << std::endl;
		}
		else if ( (*it)->step_type == Roadmap::Step::RoadStep ) {
		    
		    Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *it );

		    //
		    // retrieval of the step's geometry
		    std::string q = (boost::format("SELECT st_asbinary(st_force2d(geom)) FROM tempus.road_section WHERE id=%1%") %
				     road_graph[step->road_section].db_id ).str();
		    Db::Result res = db_.exec(q);
		    std::string wkb = res[0][0].as<std::string>();
		    // get rid of the heading '\x'
		    if ( wkb.size() > 0 )
			step->geometry_wkb = wkb.substr(2);
		    else
			step->geometry_wkb = "";
	    
		    //
		    // For a road step, we have to compute directions of turn
		    //
		    movement = Roadmap::RoadStep::GoAhead;
		
		    on_roundabout =  road_graph[step->road_section].is_roundabout;
		
		    bool action = false;
		    if ( on_roundabout && !was_on_roundabout )
		    {
			// we enter a roundabout
			roundabout_enter = road_graph[step->road_section].db_id;
			movement = Roadmap::RoadStep::RoundAboutEnter;
			action = true;
		    }
		    if ( !on_roundabout && was_on_roundabout )
		    {
			// we leave a roundabout
			roundabout_leave = road_graph[step->road_section].db_id;
			// FIXME : compute the exit number
			movement = Roadmap::RoadStep::FirstExit;
			action = true;
		    }
		
		    if ( previous_section && !on_roundabout && !action )
		    {
			std::string q1 = (boost::format("SELECT ST_Azimuth( st_endpoint(s1.geom), st_startpoint(s1.geom) ), ST_Azimuth( st_startpoint(s2.geom), st_endpoint(s2.geom) ), st_endpoint(s1.geom)=st_startpoint(s2.geom) "
							"FROM tempus.road_section AS s1, tempus.road_section AS s2 WHERE s1.id=%1% AND s2.id=%2%") % previous_section % road_graph[step->road_section].db_id).str();
			Db::Result res = db_.exec(q1);
			double pi = 3.14159265;
			double z1 = res[0][0].as<double>() / pi * 180.0;
			double z2 = res[0][1].as<double>() / pi * 180.0;
			bool continuous =  res[0][2].as<bool>();
			if ( !continuous )
			    z1 = z1 - 180;
		    
			int z = int(z1 - z2);
			z = (z + 360) % 360;
			if ( z >= 30 && z <= 150 )
			{
			    movement = Roadmap::RoadStep::TurnRight;
			}
			if ( z >= 210 && z < 330 )
			{
			    movement = Roadmap::RoadStep::TurnLeft;
			}
		    }
		
		    road_name = road_graph[step->road_section].road_name;
		    distance = road_graph[step->road_section].length;
		
		    if ( last_step )
		    {
			last_step->end_movement = movement;
			last_step->distance_km = -1.0;
		    }
		
		    switch ( movement )
		    {
		    case Roadmap::RoadStep::GoAhead:
			std::cout << direction_i++ << " - Walk on " << road_name << " for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::TurnLeft:
			std::cout << direction_i++ << " - Turn left on " << road_name << " and walk for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::TurnRight:
			std::cout << direction_i++ << " - Turn right on " << road_name << " and walk for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::RoundAboutEnter:
			std::cout << direction_i++ << " - Enter the roundabout on " << road_name << std::endl;
			break;
		    case Roadmap::RoadStep::FirstExit:
		    case Roadmap::RoadStep::SecondExit:
		    case Roadmap::RoadStep::ThirdExit:
		    case Roadmap::RoadStep::FourthExit:
		    case Roadmap::RoadStep::FifthExit:
		    case Roadmap::RoadStep::SixthExit:
			std::cout << direction_i++ << " - Leave the roundabout on " << road_name << std::endl;
			break;
		    }
		    previous_section = road_graph[step->road_section].db_id;
		    was_on_roundabout = on_roundabout;
		    last_step = step;
		}
	    }

	    if ( last_step ) {
		last_step->end_movement = Roadmap::RoadStep::YouAreArrived;
		last_step->distance_km = -1.0;
	    }
	}

	return result_;
    }

    void Plugin::cleanup()
    {
	std::cout << "[plugin_base]: cleanup" << std::endl;
    }
};

