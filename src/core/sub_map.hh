/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
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

// Sub map : a filtered std::map

#ifndef SUBMAP_HH
#define SUBMAP_HH

#include <map>
#include <set>
#include <boost/iterator/filter_iterator.hpp>

///
/// A sub_map is a specialization of std::map
/// where a subset of keys is selected for iteration.
///
/// It is used the same way a std::map is used.
/// In addition, the selected subset can be set with select( std::set<KT>& )
/// and can be iterated over with a pair of iterators given by subset_begin() and subset_end()
//
template < class KT, class VT, class Map = std::map<KT,VT> >
class sub_map : public Map {
public:
    sub_map() : Map(), predicate_( selection_ ) {}

    ///
    /// Set the current subset
    void select( const std::set<KT>& sselection ) {
        selection_ = sselection;
    }

    ///
    /// Select all the elements of the map as the current subset
    void select_all() {
        selection_.clear();
        typename Map::const_iterator it;

        for ( it = this->begin(); it != this->end(); it++ ) {
            selection_.insert( it->first );
        }
    }

    ///
    /// Set the current subset to void
    void select_none() {
        selection_.clear();
    }

    ///
    /// Retrieve current selection
    const std::set<KT>& selection() const {
        return selection_;
    }

protected:
    std::set<KT> selection_;

    template <class K, class V>
    struct FilterPredicate {
        FilterPredicate() : select_( 0 ) {}
        FilterPredicate( const std::set<K>& selection ) : select_( &selection ) {}
        bool operator() ( typename std::pair<K,V> p ) {
            if ( !select_ ) {
                return false;
            }

            return select_->find( p.first ) != select_->end();
        }
        const std::set<K>* select_;
    };
    FilterPredicate<KT, VT> predicate_;
public:
    typedef boost::filter_iterator<FilterPredicate<KT, VT>, typename Map::const_iterator> const_subset_iterator;
    typedef boost::filter_iterator<FilterPredicate<KT, VT>, typename Map::iterator> subset_iterator;
    const_subset_iterator subset_begin() const {
        return const_subset_iterator( predicate_, Map::begin(), Map::end() );
    }
    const_subset_iterator subset_end() const {
        return const_subset_iterator( predicate_, Map::end(), Map::end() );
    }
    subset_iterator subset_begin() {
        return subset_iterator( predicate_, Map::begin(), Map::end() );
    }
    subset_iterator subset_end() {
        return subset_iterator( predicate_, Map::end(), Map::end() );
    }
};

#endif

