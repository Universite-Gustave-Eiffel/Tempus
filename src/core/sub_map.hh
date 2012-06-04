// Sub map : a filtered std::map
// (c) 2012 Oslandia
// MIT License

#ifndef SUBMAP_HH
#define SUBMAP_HH

#include <map>
#include <set>
#include <boost/iterator/filter_iterator.hpp>

///
/// A sub_map is a specialization of std::map
/// where a subset of keys can is selected for iteration.
///
/// It is used the same way a std::map is used.
/// In addition, the selected subset can be set with select( std::set<KT>& )
/// and can be iterated with a pair of iterators given by subset_begin() and subset_end()
template <class KT, class VT>
class sub_map : public std::map<KT, VT>
{
public:
    sub_map() : std::map<KT, VT>(), predicate_( selection_ ) {}

    ///
    /// Set the current subset
    void select( std::set<KT>& selection )
    {
	selection_ = selection;
    }

    ///
    /// Select all the elements of the map as the current subset
    void select_all()
    {
	selection_.clear();
	typename std::map<KT,VT>::const_iterator it;
	for ( it = this->begin(); it != this->end(); it++ )
	{
	    selection_.insert( it->first );
	}
    }

    ///
    /// Set the current subset to void
    void select_none()
    {
	selection_.clear();
    }

    ///
    /// Retrieve current selection
    const std::set<KT>& selection() const
    {
	return selection_;
    }

protected:
    std::set<KT> selection_;

    template <class K, class V>
    struct FilterPredicate
    {
	FilterPredicate() : select_(0) {}
	FilterPredicate( const std::set<K>& selection ) : select_(&selection) {}
	bool operator() ( typename std::pair<K,V> p )
	{
	    if ( !select_ )
		return false;
	    return select_->find( p.first ) != select_->end();
	}
	const std::set<K>* select_;
    };
    FilterPredicate<KT, VT> predicate_;
public:
    typedef boost::filter_iterator<FilterPredicate<KT, VT>, typename std::map<KT, VT>::const_iterator> const_subset_iterator;
    typedef boost::filter_iterator<FilterPredicate<KT, VT>, typename std::map<KT, VT>::iterator> subset_iterator;
    const_subset_iterator subset_begin() const
    {
	return const_subset_iterator( predicate_, std::map<KT, VT>::begin(), std::map<KT, VT>::end() );
    }
    const_subset_iterator subset_end() const
    {
	return const_subset_iterator( predicate_, std::map<KT, VT>::end(), std::map<KT, VT>::end() );
    }
    subset_iterator subset_begin()
    {
	return subset_iterator( predicate_, std::map<KT, VT>::begin(), std::map<KT, VT>::end() );
    }
    subset_iterator subset_end()
    {
	return subset_iterator( predicate_, std::map<KT, VT>::end(), std::map<KT, VT>::end() );
    }
};

#endif

