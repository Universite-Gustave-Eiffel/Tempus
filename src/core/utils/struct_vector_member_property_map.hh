/**
 *   Copyright (C) 2012-2016 Oslandia <infos@oslandia.com>
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

#ifndef TEMPUS_UTILS_STRUCT_VECTOR_MEMBER_PROPERTY_MAP_HH
#define TEMPUS_UTILS_STRUCT_VECTOR_MEMBER_PROPERTY_MAP_HH

#include <boost/property_map/property_map.hpp>

// Util trait class to give the return type of a pointer to member
template <typename T>
struct MemberPointerR
{
    using type = void;
};

template <typename R, typename T>
struct MemberPointerR<R T::*>
{
    using type = R;
};

///
/// Adaptor class to use pointer to member in a vector of struct as a property map
template <typename VectorStruct, typename MemberPointer, typename R = typename MemberPointerR<MemberPointer>::type >
struct StructVectorMemberPropertyMap : public boost::put_get_helper< R, StructVectorMemberPropertyMap< VectorStruct, MemberPointer, R > >
{
    using Struct = typename VectorStruct::value_type;

    typedef size_t key_type;
    typedef R value_type;
    typedef value_type& reference;
    typedef boost::lvalue_property_map_tag category;

    StructVectorMemberPropertyMap( VectorStruct& v, MemberPointer p ) : v_( v ), pmember( p ) {}

    reference operator[]( size_t k ) const {
        return v_[k].*pmember;
    }

    VectorStruct& v_;
    MemberPointer pmember;
};

template <typename VectorStruct, typename MemberPointer>
StructVectorMemberPropertyMap<VectorStruct, MemberPointer> make_struct_vector_member_property_map( VectorStruct& v, MemberPointer p ) {
    return StructVectorMemberPropertyMap<VectorStruct, MemberPointer>( v, p );
}

#endif
