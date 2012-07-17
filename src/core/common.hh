// Tempus core data structures
// (c) 2012 Oslandia
// MIT License
//
// This file contains common declarations and constants used by all the object inside the "Tempus" namespace
//

#ifndef TEMPUS_COMMON_HH
#define TEMPUS_COMMON_HH

#include <map>
#include <string>
#include <iostream>
#include <boost/date_time.hpp>

#define _DEBUG

namespace Tempus
{
    ///
    /// Type used inside the DB to store IDs.
    /// O means NULL.
    ///
    typedef unsigned long long int db_id_t;

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
    /// EXPECT is used in check_consistency_() methods
    #define EXPECT( expr ) {if (!(expr)) { std::cerr << __FILE__ << ":" << __LINE__ << " Assertion " #expr " failed" << std::endl; return false; }}
    ///
    /// Pre conditions, will abort if the condition is false
    #define REQUIRE( expr ) {if (!(expr)) { std::string e_; e_ += __FILE__; e_ += ":"; e_ += __LINE__; e_ += " Precondition " #expr " is false"; throw std::runtime_error( e_ ); }}
    ///
    /// Post conditions, will abort if the condition is false
    #define ENSURE( expr ) {if (!(expr)) { std::string e_; e_ += __FILE__; e_ += ":"; e_ += __LINE__; e_ += " Postcondition " #expr " is false"; throw std::runtime_error( e_ ); }}
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
    /// Time is the number of seconds since 00:00.
    struct Time
    {
	long n_secs;
    };

    ///
    /// Date type : dd/mm/yyyy
    typedef boost::gregorian::date Date;

    ///
    /// DateTime stores a date and a time
    typedef boost::posix_time::ptime DateTime;

    ///
    /// Refers to tempus.road_type table
    struct RoadType : public Base
    {
	std::string name;
    };

    ///
    /// 2D Points
    struct Point2D
    {
	double x,y;
    };

    ///
    /// Road types constants.
    typedef std::map<db_id_t, RoadType> RoadTypes;

    ///
    /// Refers to tempus.transport_type table
    struct TransportType : public Base
    {
	// inherits from Base, the ID must be a power of 2.
	db_id_t parent_id;

	std::string name;
	
	bool need_parking;
	bool need_station;
	bool need_return;
	
    protected:
	bool check_consistency_()
	{
	    ///
	    /// x is a power of two if (x & (x - 1)) is 0
	    EXPECT( (db_id != 0) && !(db_id & (db_id - 1)) );
	    EXPECT( (parent_id != 0) && !(parent_id & (parent_id - 1)) );
	    return true;
	}
    };

    ///
    /// Transport types constants.
    typedef std::map<db_id_t, TransportType> TransportTypes;

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
	CostNumberOfChanges,
	CostVariability
    };

    ///
    /// Returns the name of a cost
    std::string cost_name( int cost );

    ///
    /// Returns the unit of a cost
    std::string cost_unit( int cost );

    ///
    /// Base class in charge of progression callback.
    class ProgressionCallback
    {
    public:
	virtual void operator()( float, bool = false )
	{
	    // Default : do nothing
	}
    };

    extern ProgressionCallback null_progression_callback;


    ///
    /// Simple progession processing: text based progression bar.
    struct TextProgression : public Tempus::ProgressionCallback
    {
    public:
	TextProgression( int N = 50 ) : N_(N), old_N_(-1)
	{
	}
	virtual void operator()( float percent, bool finished );
    protected:
	int N_;
	int old_N_;
    };

}; // Tempus namespace

#endif
