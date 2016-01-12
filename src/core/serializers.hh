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

#ifndef TEMPUS_GRAPH_SERIALIZERS_HH
#define TEMPUS_GRAPH_SERIALIZERS_HH

#include <iosfwd>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <type_traits>

#include <boost/optional.hpp>

namespace Tempus
{

struct Point2D;
struct Point3D;
class TransportMode;

namespace Road
{
struct Node;
struct Section;
}
namespace PublicTransport
{
struct Stop;
struct Section;
}
namespace Multimodal
{
struct Graph;
}

struct binary_serialization_t {};

void serialize( std::ostream& ostr, const PublicTransport::Stop& stop, binary_serialization_t t );
void unserialize( std::istream& istr, PublicTransport::Stop& stop, binary_serialization_t t );
void serialize( std::ostream& ostr, const TransportMode& tm, binary_serialization_t t );
void unserialize( std::istream& istr, TransportMode& tm, binary_serialization_t t );
void serialize( std::ostream& ostr, const Multimodal::Graph& g, binary_serialization_t t );
void unserialize( std::istream& istr, Multimodal::Graph& g, binary_serialization_t t );


void serialize( std::ostream& ostr, const char* ptr, size_t s, binary_serialization_t );
void unserialize( std::istream& istr, char* ptr, size_t s, binary_serialization_t );

template <typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
serialize( std::ostream& ostr, const T& t, binary_serialization_t b )
{
    // for POD
    serialize( ostr, (const char*)&t, sizeof(t), b );
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
unserialize( std::istream& istr, T& t, binary_serialization_t b )
{
    // for POD only
    unserialize( istr, (char*)&t, sizeof(t), b );
}

template <typename T>
typename std::enable_if<!std::is_integral<T>::value, void>::type
serialize( std::ostream& ostr, const T& t, binary_serialization_t b )
{
    // for non POD, check for a member function named serialize()
    t.serialize( ostr, b );
}

template <typename T>
typename std::enable_if<!std::is_integral<T>::value, void>::type
unserialize( std::istream& istr, T& t, binary_serialization_t b )
{
    // for non POD, check for a member function named unserialize()
    t.unserialize( istr, b );
}

template <typename K, typename V>
void serialize( std::ostream& ostr, const std::map<K,V>& m, binary_serialization_t t )
{
    size_t s = m.size();
    serialize( ostr, s, t );
    for ( auto p : m ) {
        serialize( ostr, p.first, t );
        serialize( ostr, p.second, t );
    }
}

template <typename K, typename V>
void unserialize( std::istream& istr, std::map<K,V>& m, binary_serialization_t t )
{
    m.clear();
    size_t s;
    unserialize( istr, s, t );
    for ( size_t i = 0; i < s; i++ ) {
        K k;
        V v;
        unserialize( istr, k, t );
        unserialize( istr, v, t );
        m.insert( std::make_pair(k, std::move(v)) );
    }
}

template <typename T>
void serialize( std::ostream& ostr, const boost::optional<T>& opt, binary_serialization_t t )
{
    if (opt) {
        ostr << "1";
        serialize( ostr, *opt, t );
    }
    else {
        ostr << "0";
    }
}

template <typename T>
void unserialize( std::istream& istr, boost::optional<T>& opt, binary_serialization_t t )
{
    char ok;
    istr >> ok;
    if (ok == '1') {
        T obj;
        unserialize( istr, obj, t );
        opt = obj;
    }
}

template <typename T>
void serialize( std::ostream& ostr, const std::set<T>& s, binary_serialization_t t )
{
    serialize( ostr, s.size(), t );
    for ( auto e : s ) {
        serialize( ostr, e, t );
    }
}

template <typename T>
void unserialize( std::istream& istr, std::set<T>& s, binary_serialization_t t )
{
    s.clear();
    size_t n;
    unserialize( istr, n, t );
    for ( size_t i = 0; i < n; i++ ) {
        T obj;
        unserialize( istr, obj, t );
        s.insert( std::move(obj) );
    }
}

template <typename T>
void serialize( std::ostream& ostr, const std::vector<T>& s, binary_serialization_t t )
{
    serialize( ostr, s.size(), t );
    for ( auto e : s ) {
        serialize( ostr, e, t );
    }
}

template <typename T>
void unserialize( std::istream& istr, std::vector<T>& s, binary_serialization_t t )
{
    s.clear();
    size_t n;
    unserialize( istr, n, t );
    s.reserve(n);
    for ( size_t i = 0; i < n; i++ ) {
        T e;
        unserialize( istr, e, t );
        s.push_back( std::move(e) );
    }
}

template <typename T1, typename T2>
void serialize( std::ostream& ostr, const std::pair<T1, T2>& p, binary_serialization_t t )
{
    serialize( ostr, p.first, t );
    serialize( ostr, p.second, t );
}

template <typename T1, typename T2>
void unserialize( std::istream& istr, std::pair<T1, T2>& p, binary_serialization_t t )
{
    unserialize( istr, p.first, t );
    unserialize( istr, p.second, t );
}

} // namespace Tempus

#endif
