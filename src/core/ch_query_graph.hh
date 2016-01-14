/**
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_CH_QUERY_GRAPH_HH
#define TEMPUS_CH_QUERY_GRAPH_HH

#include <type_traits>
#include <iterator>
#include "serializers.hh"

namespace Tempus
{

//
// Data structure used for storing CH graph dedicated to queries.
// It consists of an upward and a downward graph stored together.
//
// A first array (edge_index_) gives the index of first edges in a secondary array (edges_)
// for a particular node.
// Since during a CH query, adjacency of a node always consists of ascending successors or descending predecessors,
// two indices are stored : the index of the first upward edge and the index of the first downward edge.
//
// EdgeProperty: data type of each edge (usually a cost and a shortcut flag)
// EdgeIndex: type of an index in the edge array
template <typename EP,
          typename VI = uint32_t,
          typename EI = uint32_t>
class CHQueryGraph
{
public:
    typedef VI VertexIndex;
    typedef EI EdgeIndex;
    typedef EP EdgeProperty;

    // default constructor
    CHQueryGraph() {}

    //
    // @param vp iterator pointing to pair of nodes, sorted
    // @param num_vertices number of vertices
    // @param up_it iterator pointing to the number of upward edges for each vertex
    // @param ep iterator pointing to the edge property of each edge
    template <typename VertexPairIterator, typename DegreeIterator, typename EdgePropertyIterator>
    CHQueryGraph( VertexPairIterator vp,
                  size_t n_vertices,
                  DegreeIterator up_it,
                  EdgePropertyIterator ep)
    {
        static_assert( std::is_same<typename std::iterator_traits<VertexPairIterator>::value_type, std::pair<VI,VI>>::value, "Wrong VertexPairIterator type" );
        static_assert( std::is_convertible<typename std::iterator_traits<DegreeIterator>::value_type, size_t>::value, "Wrong DegreeIterator type" );
        static_assert( std::is_same<typename std::iterator_traits<EdgePropertyIterator>::value_type, EdgeProperty>::value, "Wrong EdgePropertyIterator type" );

        edge_index_.resize( n_vertices + 1 );

        for ( VI v = 0; v < n_vertices; v++ ) {
            edge_index_[v].first_upward_edge = edges_.size();
            if ( vp->first == v ) {
                // for all upward edges
                for ( size_t j = 0; j < *up_it; j++, vp++, ep++ ) {
                    EdgeData data;
                    data.target = vp->second;
                    data.property = *ep;
                    edges_.emplace_back( data );
                }
            }
            up_it++;

            edge_index_[v].first_downward_edge = edges_.size();
            if ( vp->first == v ) {
                for ( ; v == vp->first; vp++, ep++ ) {
                    EdgeData data;
                    data.target = vp->second;
                    data.property = *ep;
                    edges_.emplace_back( data );
                }
            }
        }
        edge_index_[n_vertices].first_upward_edge = edges_.size();
        edge_index_[n_vertices].first_downward_edge = edges_.size();
    }

    void debug_print( std::ostream& ostr )
    {
        for ( auto f : edge_index_ ) {
            ostr << "[" << f.first_upward_edge << "," << f.first_downward_edge << "]";
        }
        ostr << std::endl;
        for ( auto e : edges_ ) {
            ostr << e.target << "|";
        }
        ostr << std::endl;
    }

    struct EdgeData
    {
        VertexIndex target;
        EdgeProperty property;
        void serialize( std::ostream& ostr, binary_serialization_t t ) const
        {
            Tempus::serialize( ostr, target, t );
            Tempus::serialize( ostr, property, t );
        }
        void unserialize( std::istream& istr, binary_serialization_t t )
        {
            Tempus::unserialize( istr, target, t );
            Tempus::unserialize( istr, property, t );
        }
    };

    class EdgeDescriptor
    {
    public:
        EdgeDescriptor() : prop_(nullptr) {}
        EdgeDescriptor( VertexIndex src, VertexIndex tgt, const EdgeProperty* prop, bool upward ) : source_(src), target_(tgt), prop_(prop), upward_(upward) {}
        VertexIndex source() const { return source_; }
        VertexIndex target() const { return target_; }
        const EdgeProperty& property() const { return *prop_; }
        bool is_upward() const { return upward_; }
    private:
        VertexIndex source_, target_;
        const EdgeProperty* prop_;
        bool upward_;
    };

    typedef EdgeDescriptor edge_descriptor;

    class OutEdgeIterator
    {
    public:
        OutEdgeIterator( VertexIndex source, const EdgeData* data ) : source_(source), data_(data), v_( source, data->target, &data->property, true ) {}
        EdgeDescriptor operator*() { return v_; }
        EdgeDescriptor* operator->() { return &v_; }
        void operator++() { data_++; v_ = EdgeDescriptor(source_, data_->target, &data_->property, true); }
        void operator++(int) { this->operator++(); }
        bool operator==( const OutEdgeIterator& other ) const { return other.source_ == source_ && other.data_ == data_; }
        bool operator!=( const OutEdgeIterator& other ) const { return !(*this == other); }
    private:
        VertexIndex source_;
        const EdgeData* data_;
        EdgeDescriptor v_;
    };

    class InEdgeIterator
    {
    public:
        InEdgeIterator( VertexIndex source, const EdgeData* data ) : source_(source), data_(data), v_( data->target, source_, &data_->property, false ) {}
        EdgeDescriptor operator*() { return v_; }
        EdgeDescriptor* operator->() { return &v_; }
        void operator++() { data_++; v_ = EdgeDescriptor(data_->target, source_, &data_->property, false ); }
        void operator++(int) { this->operator++(); }
        bool operator==( const InEdgeIterator& other ) const { return other.source_ == source_ && other.data_ == data_; }
        bool operator!=( const InEdgeIterator& other ) const { return !(*this == other); }
    private:
        VertexIndex source_;
        const EdgeData* data_;
        EdgeDescriptor v_;
    };

    std::pair<OutEdgeIterator, OutEdgeIterator> out_edges( VertexIndex v ) const
    {
        return std::make_pair( OutEdgeIterator( v, &edges_[edge_index_[v].first_upward_edge] ),
                               OutEdgeIterator( v, &edges_[edge_index_[v].first_downward_edge] ) );
    }

    size_t out_degree( VertexIndex v ) const
    {
        return edge_index_[v].first_downward_edge - edge_index_[v].first_upward_edge;
    }

    std::pair<InEdgeIterator, InEdgeIterator> in_edges( VertexIndex v ) const
    {
        return std::make_pair( InEdgeIterator( v, &edges_[edge_index_[v].first_downward_edge] ),
                               InEdgeIterator( v, &edges_[edge_index_[v+1].first_upward_edge] ) );
    }

    size_t in_degree( VertexIndex v ) const
    {
        return edge_index_[v+1].first_upward_edge - edge_index_[v].first_downward_edge;
    }

    size_t num_vertices() const { return edge_index_.size() - 1; }

    class EdgeIterator
    {
    public:
        EdgeIterator( VertexIndex source, EdgeIndex edge, CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
            :
            source_(source), edge_(edge), graph_(graph),
            v_( source_, graph_.edges_[edge_].target, &graph_.edges_[edge_].property, true )
        {
            update_();
        }
            
        void operator++()
        {
            edge_++;
            update_();
        }

        void operator++(int) { this->operator++(); }
        bool operator==( const EdgeIterator& other ) const
        {
            return /*other.source_ == source_ && */other.edge_ == edge_;
        }
        bool operator!=( const EdgeIterator& other ) const
        {
            return ! this->operator==(other);
        }
        
        EdgeDescriptor& operator*() { return v_; }
        EdgeDescriptor* operator->() { return &v_; }
        
    private:
        void update_()
        {
            bool out = true;
            while ( source_ < graph_.edge_index_.size() ) {
                if ( edge_ < graph_.edge_index_[source_].first_downward_edge ) {
                    out = true;
                    break;
                }
                else if ( edge_ < graph_.edge_index_[source_+1].first_upward_edge ) {
                    out = false;
                    break;
                }
                // else
                source_++;
            }

            if ( out ) {
                v_ = EdgeDescriptor( source_, graph_.edges_[edge_].target, &graph_.edges_[edge_].property, true );
            }
            else {
                v_ = EdgeDescriptor( graph_.edges_[edge_].target, source_, &graph_.edges_[edge_].property, false );
            }
        }
        VertexIndex source_;
        EdgeIndex edge_;
        CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph_;
        EdgeDescriptor v_;
    };

    std::pair<EdgeIterator, EdgeIterator> edges()
    {
        return std::make_pair( EdgeIterator( 0, 0, *this ), EdgeIterator( edge_index_.size()-1, edges_.size(), *this ) );
    }

    std::pair<EdgeDescriptor, bool> edge( VertexIndex u, VertexIndex v ) const
    {
        if ( v > u ) {
            EdgeIndex s = edge_index_[u].first_upward_edge;
            EdgeIndex t = edge_index_[u].first_downward_edge;
            for ( EdgeIndex i = s; i < t; i++ ) {
                if ( edges_[i].target == v ) {
                    return std::make_pair(EdgeDescriptor( u, v, &edges_[i].property, true ), true);
                }
            }
        }
        if ( u > v ) {
            EdgeIndex s = edge_index_[v].first_downward_edge;
            EdgeIndex t = edge_index_[v+1].first_upward_edge;
            for ( EdgeIndex i = s; i < t; i++ ) {
                if ( edges_[i].target == u ) {
                    return std::make_pair(EdgeDescriptor( u, v, &edges_[i].property, false ), true);
                }
            }
        }
        return std::make_pair(EdgeDescriptor( u, v, nullptr, true ), false );
    }

    void serialize( std::ostream& ostr, binary_serialization_t t ) const
    {
        EdgeIndex n = edge_index_.size();
        Tempus::serialize( ostr, n, t );
        Tempus::serialize( ostr, reinterpret_cast<const char*>( &edge_index_[0] ), n * sizeof( FirstEdgeIndex ), t );
        EdgeIndex m = edges_.size();
        Tempus::serialize( ostr, m, t );
        for ( EdgeIndex i = 0; i < m; i++ )
            Tempus::serialize( ostr, edges_[i], t );
    }
        
    void unserialize( std::istream& istr, binary_serialization_t t )
    {
        EdgeIndex n = 0;
        Tempus::unserialize( istr, n, t );
        edge_index_.resize( n );
        Tempus::unserialize( istr, reinterpret_cast<char*>( &edge_index_[0] ), n * sizeof( FirstEdgeIndex ), t );
        EdgeIndex m = 0;
        Tempus::unserialize( istr, m, t );
        edges_.resize( m );
        for ( EdgeIndex i = 0; i < m; i++ )
            Tempus::unserialize( istr, edges_[i], t );
    }
private:
    //FIXME : only one index and a local linear search for out_edges and in_edges may be sufficient
    struct FirstEdgeIndex {
        EdgeIndex first_upward_edge;
        EdgeIndex first_downward_edge;
    };

    std::vector<FirstEdgeIndex> edge_index_;
    std::vector<EdgeData> edges_;
};

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
VertexIndex source( const typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::EdgeDescriptor& e, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& )
{
    return e.source();
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
VertexIndex target( const typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::EdgeDescriptor& e, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& )
{
    return e.target();
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
std::pair<typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::OutEdgeIterator,
          typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::OutEdgeIterator> out_edges( VertexIndex v, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.out_edges( v );
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
std::pair<typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::InEdgeIterator,
          typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::InEdgeIterator> in_edges( VertexIndex v, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.in_edges( v );
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
std::pair<typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::EdgeIterator,
          typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::EdgeIterator> edges( CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.edges();
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
size_t num_vertices( const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.num_vertices();
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
size_t out_degree( VertexIndex v, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.out_degree( v );
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
size_t in_degree( VertexIndex v, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.in_degree( v );
}

template <typename EdgeProperty,
          typename VertexIndex = uint32_t,
          typename EdgeIndex = uint32_t>
std::pair<typename CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>::EdgeDescriptor, bool>
edge( VertexIndex u, VertexIndex v, const CHQueryGraph<EdgeProperty, VertexIndex, EdgeIndex>& graph )
{
    return graph.edge( u, v );
}


}

#endif
