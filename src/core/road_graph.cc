#include "road_graph.hh"

namespace Tempus
{
    namespace Road
    {
        Road::VertexSequence Road::to_vertex_sequence( const Graph& graph ) const
        {
            Road::VertexSequence seq;
            if ( road_sections.size() == 0 ) {
                return seq;
            }
            if ( road_sections.size() == 1 ) {
                // should not happen, push back in an arbitrary order
                Edge edge( road_sections[0] );
                seq.push_back( source( edge, graph ) );
                seq.push_back( target( edge, graph ) );
                return seq;
            }

            Edge edge1( road_sections[0] );
            Edge edge2( road_sections[1] );
            if ( (target( edge1, graph ) == source( edge2, graph )) ||
                 (target( edge1, graph ) == target( edge2, graph )) ) {
                seq.push_back( source( edge1, graph ) );
                seq.push_back( target( edge1, graph ) );
            }
            else if ( (source( edge1, graph ) == source( edge2, graph )) ||
                      (source( edge1, graph ) == target( edge2, graph )) ) {
                seq.push_back( target( edge1, graph ) );
                seq.push_back( source( edge1, graph ) );
            }

            for ( size_t i = 1; i < road_sections.size(); ++i ) {
                Edge edge( road_sections[i] );
                if ( source( edge, graph ) == seq.back() ) {
                    seq.push_back( target( edge, graph ) );
                }
                else {
                    seq.push_back( source( edge, graph ) );
                }
            }
            return seq;
        }
    }
}
