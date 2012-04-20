#include "common.hh"

namespace Tempus
{
    ///
    /// Declaration of global arrays
    RoadTypes road_types;
    TransportTypes transport_types;
    NameToId road_type_from_name;
    NameToId transport_type_from_name;

    ProgressionCallback null_progression_callback;

    void TextProgression::operator()( float percent, bool finished )
    {
	// only update when changes are visible
	int new_N = int(percent * N_);
	if ( new_N == old_N_ )
	    return;
	
	if ( finished || (old_N_ == -1) )
	{
	    std::cout << "\r";
	    std::cout << "[";
	    for (int i = 0; i < new_N; i++)
		std::cout << ".";
	    for (int i = new_N; i < N_; i++)
		std::cout << " ";
	    std::cout << "] ";
	    std::cout << int(percent * 100) << "%";
	    if ( finished )
		std::cout << std::endl;
	    std::cout.flush();
	}
	old_N_ = new_N;
    }
};
