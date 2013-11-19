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

//
// Adaptor to make a std::map a boost::property_map
// The slight difference with boost::associative_property_map is
// that we support here a default value for non existing keys
//
// This way, there is no need to fill a distance_map with +inf on each node
// +inf could be considered as the default value
template <class UniquePairAssociativeContainer>
class associative_property_map_default_value
{
    typedef UniquePairAssociativeContainer C;
public:
    typedef typename C::key_type key_type;
    typedef typename C::value_type::second_type value_type;
    typedef value_type& reference;
    typedef boost::read_write_property_map_tag category;

    associative_property_map_default_value( const value_type& defaultv ) :
        c_(0),
        default_(defaultv)
    {}
    associative_property_map_default_value( C& c, const value_type& defaultv ) :
        c_(&c),
        default_(defaultv)
    {}

    value_type get( const key_type& k ) const
    {
        typename C::const_iterator it = c_->find( k );
        if ( it == c_->end() ) {
            return default_;
        }
        return it->second;
    }
    void put( const key_type& k, const value_type& v )
    {
        (*c_)[k] = v;
    }
private:
    C* c_;
    value_type default_;
};

template <class Map>
typename Map::value_type::second_type get( const associative_property_map_default_value<Map>& pa, const typename Map::key_type& k )
{
    return pa.get( k );
}

template <class Map>
void put( associative_property_map_default_value<Map>& pa, typename Map::key_type k, const typename Map::value_type::second_type& v )
{
    return pa.put( k, v );
}

