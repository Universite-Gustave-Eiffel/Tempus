// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A MultimodalGraph is a Road::Graph and a list of PublicTransport::Graph
 */

#ifndef TEMPUS_MULTIMODAL_GRAPH_HH
#define TEMPUS_MULTIMODAL_GRAPH_HH

#include "common.hh"
#include "road_graph.hh"
#include "public_transport_graph.hh"

namespace Tempus
{
    ///
    /// A MultimodalGraph is basically a Road::Graph associated with a list of PublicTransport::Graph
    struct MultimodalGraph
    {
	Road::Graph road;
	
	typedef std::list<PublicTransport::Graph> PublicTransportGraphList;
	PublicTransportGraphList public_transports;
    };
}; // Tempus namespace

#endif
