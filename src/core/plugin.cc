#include <string>
#include <iostream>
#include <memory>

#include <boost/format.hpp>

#include "plugin.hh"
#include "utils/graph_db_link.hh"


namespace Tempus
{

    PluginFactory::~PluginFactory()
    {
        for ( DllMap::iterator i=dll_.begin(); i!=dll_.end(); i++)
        {
#ifdef _WIN32
            FreeLibrary( i->second.handle_ );
#else
            if ( dlclose( i->second.handle_ ) )
            {
                CERR << "Error on dlclose " << dlerror() << std::endl;
            }
#endif
        }
    }

    PluginFactory PluginFactory::instance;

    void PluginFactory::load( const std::string& dll_name )
    {
        if ( dll_.find(dll_name) != dll_.end() ) return;// already there 
        const std::string complete_dll_name = DLL_PREFIX + dll_name + DLL_SUFFIX;
        COUT << "Loading " << complete_dll_name << std::endl;
#ifdef _WIN32
        HMODULE h = LoadLibrary( complete_dll_name.c_str() );
        if ( h == NULL )
        {
            throw std::runtime_error("DLL loading problem: "+std::string(GetLastError()));
        }
        Dll::PluginCreationFct createFct = (Dll::PluginCreationFct) GetProcAddress( h, "createPlugin" );
        if ( createFct == NULL )
        {
            throw std::runtime_error("GetProcAddress problem: "+std::string(GetLastError()));
            FreeLibrary( h );
        }
        Dll::PluginOptionDescriptionFct optDescFct = (Dll::PluginOptionDescriptionFct) GetProcAddress( h, "optionDescriptions" );
        if ( optDescFct == NULL )
        {
            throw std::runtime_error("GetProcAddress problem: "+std::string(GetLastError()));
            FreeLibrary( h );
        }
        Dll::PluginNameFct nameFct = (Dll::PluginNameFct) GetProcAddress( h, "plugin_name" );
        if ( nameFct == NULL )
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
        Dll::PluginCreationFct createFct = (Dll::PluginCreationFct)dlsym( h, "createPlugin" );
        if ( createFct == 0 )
        {
            dlclose( h );
            throw std::runtime_error( dlerror() );
        }
        Dll::PluginOptionDescriptionFct optDescFct = (Dll::PluginOptionDescriptionFct)dlsym( h, "optionDescriptions" );
        if ( optDescFct == 0 )
        {
            dlclose( h );
            throw std::runtime_error( dlerror() );
        }
        Dll::PluginNameFct nameFct = (Dll::PluginNameFct)dlsym( h, "pluginName" );
        if ( nameFct == 0 )
        {
            dlclose( h );
            throw std::runtime_error( dlerror() );
        }
#endif
        Dll dll = { h, createFct, optDescFct };
        const std::string pluginName = (*nameFct)();
        dll_.insert( std::make_pair( pluginName, dll ) );
        COUT << "loaded " << pluginName << " from " << dll_name << "\n";
    }

    std::vector<std::string> PluginFactory::plugin_list() const
    {
        std::vector<std::string> names;
        for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++)
            names.push_back( i->first );
        return names;
    }

    Plugin * PluginFactory::createPlugin( const std::string & dll_name ) const
    {
        std::string loaded;
        for (DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++)
            loaded += " " + i->first;
        DllMap::const_iterator dll = dll_.find(dll_name);
        if ( dll == dll_.end() ) throw std::runtime_error(dll_name + " is not loaded (loaded:" + loaded+")");
        std::auto_ptr<Plugin> p( dll->second.create( Application::instance()->db_options() ) );
        p->post_build();
        p->validate();
        return p.release();
    }
    
    const Plugin::OptionDescriptionList PluginFactory::option_descriptions( const std::string & dll_name ) const
    {
        std::string loaded;
        for (DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++)
            loaded += " " + i->first;
        DllMap::const_iterator dll = dll_.find(dll_name);
        if ( dll == dll_.end() ) throw std::runtime_error(dll_name + " is not loaded (loaded:" + loaded+")");
        std::auto_ptr<const Plugin::OptionDescriptionList> list( dll->second.options_description() );  
        return Plugin::OptionDescriptionList(*list);
    }
    

    Plugin::Plugin( const std::string& nname, const std::string & db_options ) :
	graph_(Application::instance()->graph()),
	name_(nname),
	db_(db_options) // create another connection
    {
	// default metrics
	metrics_[ "time_s" ] = (float)0.0;
	metrics_[ "iterations" ] = (int)0;
    }

    template <class T>
    void Plugin::get_option( const std::string& nname, T& value )
    {
        const OptionDescriptionList desc = PluginFactory::instance.option_descriptions( name_ );
        OptionDescriptionList::const_iterator descIt = desc.find( nname );
        if ( descIt == desc.end() )
        {
            throw std::runtime_error( "get_option(): cannot find option " + nname );
        }
        if ( options_.find( nname ) == options_.end() )
        {
            // return default value
            value = boost::any_cast<T>( descIt->second.default_value );
            return;
        }
        boost::any v = options_[nname];
        if ( !v.empty() )
        {
            value = boost::any_cast<T>(options_[nname]);
        }
    }
    // template instanciations
    template void Plugin::get_option<bool>( const std::string&, bool& );
    template void Plugin::get_option<int>( const std::string&, int& );
    template void Plugin::get_option<double>( const std::string&, double& );
    template void Plugin::get_option<std::string>( const std::string&, std::string& );

    void Plugin::set_option_from_string( const std::string& nname, const std::string& value)
    {
        const Plugin::OptionDescriptionList desc = PluginFactory::instance.option_descriptions( name_ );
        Plugin::OptionDescriptionList::const_iterator descIt = desc.find( nname );
	if ( descIt == desc.end() )
	    return;
	const OptionType t = descIt->second.type;
        try {
            switch (t)
            {
            case BoolOption:
                options_[nname] = lexical_cast<int>( value ) == 0 ? false : true;
                break;
            case IntOption:
                options_[nname] = lexical_cast<int>( value );
                break;
            case FloatOption:
                options_[nname] = lexical_cast<float>( value );
                break;
            case StringOption:
                options_[nname] = value;
                break;
            default:
                throw std::runtime_error( "Unknown type" );
            }
        }
        catch ( boost::bad_lexical_cast& ) {
            options_[nname] = descIt->second.default_value;
        }
    }

    std::string Plugin::option_to_string( const std::string& nname )
    {
        const Plugin::OptionDescriptionList desc = PluginFactory::instance.option_descriptions( name_ );
        Plugin::OptionDescriptionList::const_iterator descIt = desc.find( nname );
	if ( descIt == desc.end() )
	{
	    throw std::invalid_argument( "Cannot find option " + nname );
	}
	OptionType t = descIt->second.type;
	boost::any value = options_[nname];
	if ( value.empty() )
	    return "";

	switch (t)
	{
	case BoolOption:
	    return to_string( boost::any_cast<bool>(value) ? 1 : 0 );
	case IntOption:
	    return to_string( boost::any_cast<int>(value) );
	case FloatOption:
	    return to_string( boost::any_cast<float>(value) );
	case StringOption:
	    return boost::any_cast<std::string>(value);
	default:
	    throw std::runtime_error( "Unknown type" );
	}
	// never happens
	return "";
    }

    std::string Plugin::metric_to_string( const std::string& nname )
    {
	if ( metrics_.find( nname ) == metrics_.end() )
	{
	    throw std::invalid_argument( "Cannot find metric " + nname );
	}
	boost::any v = metrics_[nname];
	if ( v.empty() )
	    return "";

	if ( v.type() == typeid( bool ) )
	{
	    return to_string( boost::any_cast<bool>( v ) ? 1 : 0 );
	}
	else if ( v.type() == typeid( int ) )
	{
	    return to_string( boost::any_cast<int>( v ) );
	}
	else if ( v.type() == typeid( float ) )
	{
	    return to_string( boost::any_cast<float>( v ) );
	}
	else if ( v.type() == typeid( std::string ) )
	{
	    return boost::any_cast<std::string>( v );
	}
	throw std::invalid_argument( "No known conversion for metric " + nname );
	// never happens
	return "";
    }

    void Plugin::post_build()
    {
	COUT << "[plugin_base]: post_build" << std::endl;
    }

    void Plugin::validate()
    {
	COUT << "[plugin_base]: validate" << std::endl;
    }

    void Plugin::cycle()
    {
	COUT << "[plugin_base]: cycle" << std::endl;
    }

    void Plugin::pre_process( Request& request ) throw (std::invalid_argument)
    {
	COUT << "[plugin_base]: pre_process" << std::endl;
	request_ = request;
	result_.clear();
    }

    ///
    /// Process the user request.
    /// Must populates the 'result_' object.
    void Plugin::process()
    {
	COUT << "[plugin_base]: process" << std::endl;
    }

    void Plugin::post_process()
    {
	COUT << "[plugin_base]: post_process" << std::endl;
    }

    ///
    /// Text formatting and preparation of roadmap
    Result& Plugin::result()
    {
	for ( Result::iterator rit = result_.begin(); rit != result_.end(); ++rit ) {
	    Roadmap& roadmap = *rit;
	    Road::Graph& road_graph = graph_.road;

#if DO_PRINT
	    // display the global costs of the result
	    for ( Costs::const_iterator it = roadmap.total_costs.begin(); it != roadmap.total_costs.end(); ++it ) {
		COUT << "Total " << cost_name( it->first ) << ": " << it->second << cost_unit( it->first ) << std::endl;
	    }
#endif
	    
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
			const Road::Graph& rroad_graph = *(edge->source.road_graph);
			const PublicTransport::Graph& pt_graph = *(edge->target.pt_graph);
			road_id = rroad_graph[ edge->source.road_vertex ].db_id;
			pt_id = pt_graph[ edge->target.pt_vertex ].db_id;

#if DO_PRINT
			COUT << direction_i++ << " - Go to the station " << pt_graph[ edge->target.pt_vertex ].name << std::endl;
#endif

		    } break;
		    case Multimodal::Edge::Transport2Road: {
			is_road_pt = true;
			const PublicTransport::Graph& pt_graph = *(edge->source.pt_graph);
			const Road::Graph& rroad_graph = *(edge->target.road_graph);
			pt_id = pt_graph[ edge->source.pt_vertex ].db_id;
			road_id = rroad_graph[ edge->target.road_vertex ].db_id;				 

#if DO_PRINT
			COUT << direction_i++ << " - Leave the station " << pt_graph[ edge->source.pt_vertex ].name << std::endl;
#endif

		    } break;
                    default: break;
		    }

		    if ( is_road_pt ) {
			std::string query = (boost::format( "SELECT st_asbinary(st_makeline(t1.geom, t2.geom)) from "
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
		    std::string q = (boost::format("SELECT st_asbinary(geom) FROM tempus.pt_section WHERE stop_from=%1% AND stop_to=%2%") %
				     pt_graph[step->section].stop_from % pt_graph[step->section].stop_to ).str();
		    Db::Result res = db_.exec(q);
		    std::string wkb = res[0][0].as<std::string>();
		    // get rid of the heading '\x'
		    step->geometry_wkb = wkb.substr(2);
		    
#if DO_PRINT
		    PublicTransport::Vertex v1 = vertex_from_id( pt_graph[step->section].stop_from, pt_graph );
		    PublicTransport::Vertex v2 = vertex_from_id( pt_graph[step->section].stop_to, pt_graph );
		    COUT << direction_i++ << " - Take the trip #" << step->trip_id << " from '" << pt_graph[v1].name << "' to '" << pt_graph[v2].name << "' (";
		    // display associated costs
		    for ( Costs::const_iterator cit = step->costs.begin(); cit != step->costs.end(); ++cit ) {
			COUT << cost_name( cit->first ) << ": " << cit->second << cost_unit( cit->first ) << " ";
		    }
		    COUT << ")" << std::endl;
#endif
		}
		else if ( (*it)->step_type == Roadmap::Step::RoadStep ) {
		    
		    Roadmap::RoadStep* step = static_cast<Roadmap::RoadStep*>( *it );


		    //
		    // retrieval of the step's geometry
                    {
                        std::string q = (boost::format("SELECT st_asbinary(geom) FROM tempus.road_section WHERE id=%1%") %
                                         road_graph[step->road_section].db_id ).str();
                        Db::Result res = db_.exec(q);
                        std::string wkb = res[0][0].as<std::string>();
                        // get rid of the heading '\x'
                        if ( wkb.size() > 0 )
                            step->geometry_wkb = wkb.substr(2);
                        else
                            step->geometry_wkb = "";
                    }
	    
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
		
#if DO_PRINT
		    switch ( movement )
		    {
		    case Roadmap::RoadStep::GoAhead:
			COUT << direction_i++ << " - Walk on " << road_name << " for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::TurnLeft:
			COUT << direction_i++ << " - Turn left on " << road_name << " and walk for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::TurnRight:
			COUT << direction_i++ << " - Turn right on " << road_name << " and walk for " << distance << cost_unit(CostDistance) << std::endl;
			break;
		    case Roadmap::RoadStep::RoundAboutEnter:
			COUT << direction_i++ << " - Enter the roundabout on " << road_name << std::endl;
			break;
		    case Roadmap::RoadStep::FirstExit:
		    case Roadmap::RoadStep::SecondExit:
		    case Roadmap::RoadStep::ThirdExit:
		    case Roadmap::RoadStep::FourthExit:
		    case Roadmap::RoadStep::FifthExit:
		    case Roadmap::RoadStep::SixthExit:
			COUT << direction_i++ << " - Leave the roundabout on " << road_name << std::endl;
			break;
                    case Roadmap::RoadStep::UTurn:
                        COUT << direction_i++ << " - Make a U-turn" << std::endl;
                        break;
                    case Roadmap::RoadStep::YouAreArrived:
                        COUT << direction_i++ << " - You are arrived" << std::endl;
                        break;
		    }
#endif
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
	COUT << "[plugin_base]: cleanup" << std::endl;
    }
}

