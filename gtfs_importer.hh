#ifndef TEMPUS_GTFS_IMPORTER_HH
#define TEMPUS_GTFS_IMPORTER_HH

#include "public_transport_graph.hh"

namespace Tempus
{
    class GTFSImporter
    {
    public:
	static void import( const char* dir_name );
    };
};

#endif
