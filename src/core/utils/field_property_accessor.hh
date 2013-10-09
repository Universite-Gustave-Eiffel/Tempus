// Tempus core data structures
// (c) 2013 Oslandia
// MIT License

namespace Tempus
{
    ///
    /// A FieldPropertyAccessor implements a Readable Property Map concept and gives read access
    /// to the member of a vertex or edge
    template <class Graph, class Tag, class T, class Member>
    struct FieldPropertyAccessor
    {
	typedef T value_type;
	typedef T& reference;
	typedef typename Tempus::vertex_or_edge<Graph, Tag>::descriptor key_type;
	typedef boost::readable_property_map_tag category;

	FieldPropertyAccessor( Graph& graph, Member mem ) : graph_(graph), mem_(mem) {}
	Graph& graph_;
	Member mem_;
    };
}

namespace boost
{
    ///
    /// Implementation of FieldPropertyAccessor
    template <class Graph, class Tag, class T, class Member>
    T get( Tempus::FieldPropertyAccessor<Graph, Tag, T, Member> pmap, typename Tempus::vertex_or_edge<Graph, Tag>::descriptor e )
    {
	return pmap.graph_[e].*(pmap.mem_);
    }
}


