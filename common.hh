// Tempus core data structures
// (c) 2012 Oslandia
// MIT License
/**
   This file contains common declarations and constants used by all the object inside the "Tempus" namespace
 */

#ifndef TEMPUS_COMMON_HH
#define TEMPUS_COMMON_HH

#include <map>
#include <string>

#define DEBUG

namespace Tempus
{
    ///
    /// Type used inside the DB to store IDs
    ///
    typedef long int db_id_t;

    struct Base
    {
	///
	/// Persistant ID relative to the storage database.
	/// Common to many classes.
	db_id_t db_id;

	///
	/// Consistency checking.
	/// When on debug mode, declare a virtual function that could be overloaded
	/// in derived classes.
	/// When the debug mode is disabled, no vtable is created.
#ifdef DEBUG
	virtual bool check_consistency() { return true; }
#else
	bool check_consistency() { return true; }
#endif
    };
#define EXPECT( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Assertion " #expr " failed" << std::endl; return false; }}

    ///
    /// Time is the number of seconds since 00:00
    typedef int Time;

    ///
    /// Date type : dd/mm/yyyy
    struct Date
    {
	unsigned char day;
	unsigned char month;
	unsigned short year;
    };

    ///
    /// Road type enumeration
    enum RoadType
    {
	RoadMotorway = 1,
	RoadPrimary,
	RoadSecondary,
	RoadStreet,
	RoadOther,
	RoadCycleWay,
	RoadPedestrial
    };

    ///
    /// Transport type enumeration
    struct TransportType
    {
	db_id_t id; ///< must be a power of 2
	db_id_t parent_id;

	std::string name;
	
	bool need_parking;
	bool need_station;
	bool need_return;

	TransportType() {}
	TransportType( db_id_t id, db_id_t parent_id, std::string name, bool need_parking, bool need_station, bool need_return ) :
	    id(id), parent_id(parent_id), name(name), need_parking(need_parking), need_station(need_station), need_return(need_return) {}
    };

    ///
    /// IDs of transport types, powers of 2
    enum TransportTypeId
    {
	TransportCar = (1 << 0),
	TransportPedestrial = (1 << 1),
	TransportCycle = (1 << 2),
	TransportBus = (1 << 3),
	TransportTramway = (1 << 4),
	TransportMetro = (1 << 5),
	TransportTrain = (1 << 6),
	TransportSharedCycle = (1 << 7),
	TransportSharedCar = (1 << 8),
	TransportRoller = (1 << 9)
    };

    // forward declaration
    std::map<db_id_t, TransportType> init_transport_types_();

    ///
    /// Transport types constants.
    /// This is made through a static function used to initialize a static map.
    const std::map<db_id_t, TransportType> transport_types = init_transport_types_();

    std::map<db_id_t, TransportType> init_transport_types_()
    {
	std::map<db_id_t, TransportType> t;
	t[ TransportCar ] = TransportType( TransportCar, 0, "Car", true, false, false ); 
	t[ TransportPedestrial ] = TransportType( TransportPedestrial, 0, "Pedestrial", false, false, false ); 
	t[ TransportCycle ] = TransportType( TransportCycle, 0, "Cycle", true, false, false ); 
	t[ TransportBus ] = TransportType( TransportBus, 0, "Bus", false, false, false ); 
	t[ TransportTramway ] = TransportType( TransportTramway, 0, "Tramway", false, false, false ); 
	t[ TransportMetro ] = TransportType( TransportMetro, 0, "Metro", false, false, false ); 
	t[ TransportTrain ] = TransportType( TransportTrain, 0, "Train", false, false, false ); 
	t[ TransportSharedCycle ] = TransportType( TransportSharedCycle, TransportCycle, "Shared cycle", true, true, false ); 
	t[ TransportSharedCar ] = TransportType( TransportSharedCar, TransportCar, "Shared car", true, true, false ); 
	t[ TransportRoller ] = TransportType( TransportRoller, TransportPedestrial | TransportCycle, "Roller", false, false, false ); 
	return t;
    }
}; // Tempus namespace

#endif
