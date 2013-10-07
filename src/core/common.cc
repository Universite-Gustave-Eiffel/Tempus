#include "common.hh"


namespace Tempus
{
    boost::mutex iostream_mutex;

    ProgressionCallback null_progression_callback;

    void TextProgression::operator()( float percent, bool finished )
    {
	// only update when changes are visible
	int new_N = int(percent * N_);
	if ( new_N == old_N_ )
	    return;
	
	if ( finished || (old_N_ == -1) )
	{
	    COUT << "\r";
	    COUT << "[";
	    for (int i = 0; i < new_N; i++)
		COUT << ".";
	    for (int i = new_N; i < N_; i++)
		COUT << " ";
	    COUT << "] ";
	    COUT << int(percent * 100) << "%";
	    if ( finished )
		COUT << std::endl;
	    COUT.flush();
	}
	old_N_ = new_N;
    }

    std::string cost_name( int cost )
    {
	switch ( cost ) {
	case CostDistance:
	    return "distance";
	case CostDuration:
	    return "duration";
	case CostPrice:
	    return "price";
	case CostCarbon:
	    return "carbon";
	case CostCalories:
	    return "calories";
	case CostNumberOfChanges:
	    return "number of changes";
	case CostVariability:
	    return "variability";
	}
	return "--";
    }

    std::string cost_unit( int cost )
    {
	switch ( cost ) {
	case CostDistance:
	    return "m";
	case CostDuration:
	    return "min";
	case CostPrice:
	    return "€";
	case CostCarbon:
	    return "g/m^3";
	case CostCalories:
	    return "kcal";
	}
	return "";
    }
};
