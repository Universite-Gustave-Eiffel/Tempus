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

#define _DEBUG

namespace Tempus
{
    ///
    /// Type used inside the DB to store IDs
    ///
    typedef long int db_id_t;

    struct ConsistentClass
    {
	///
	/// Consistency checking.
	/// When on debug mode, calls the virtual check() method.
	/// When the debug mode is disabled, it does nothing.
	bool check_consistency()
	{
#ifdef _DEBUG
	    return check_consistency_();
#endif
	}
    protected:
	///
	/// Private method to override in derived classes. Does nothing by default.
	virtual bool check_consistency_() { return true; }
    };

#ifdef _DEBUG
    ///
    /// EXPECT is used in check_concistency_() methods
    #define EXPECT( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Assertion " #expr " failed" << std::endl; return false; }}
    ///
    /// Pre conditions, will abort if the condition is false
    #define REQUIRE( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Precondition " #expr " is false" << std::endl; std::abort(); }}
    ///
    /// Post conditions, will abort if the condition is false
    #define ENSURE( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Postcondition " #expr " is false" << std::endl; std::abort(); }}
#else
#define EXPECT( expr ) ((void)0)
#define REQUIRE( expr ) ((void)0)
#define ENSURE( expr ) ((void)0)
#endif

    struct Base : public ConsistentClass
    {
	///
	/// Persistant ID relative to the storage database.
	/// Common to many classes.
	db_id_t db_id;
    };

    ///
    /// Time is the number of seconds since 00:00
    struct Time
    {
	long n_secs;
    };

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

    ///
    /// Transport types constants.
    /// This is made through a static function used to initialize a static map.
    extern const std::map<db_id_t, TransportType> transport_types;

    ///
    /// Type used to model costs. Either in a Step or as an optimizing criterion.
    /// This is a map to a double value and thus is user extensible.
    typedef std::map<int, double> Costs;

    ///
    /// Default common cost identifiers
    enum CostId
    {
	CostDistance = 1,
	CostDuration,
	CostPrice,
	CostCarbon,
	CostCalories,
	CostNumberOfChanges
    };

}; // Tempus namespace

#endif
