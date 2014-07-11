/**
 *   Copyright (C) 2012-2014 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2014 Oslandia <infos@oslandia.com>
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

#ifndef TEMPUS_PROPERTY_HH
#define TEMPUS_PROPERTY_HH

namespace Tempus
{

template <typename T>
struct remove_const
{
    typedef T type;
};
template <typename T>
struct remove_const<T const>
{
    typedef T type;
};

///
/// Macro used to declare a class property as well as its getter and setter
#define DECLARE_RW_PROPERTY(NAME, TYPE)                \
private:                                               \
  TYPE NAME ## _;                                      \
public:                                                \
  const remove_const<TYPE>::type& NAME() const { return NAME ## _; } \
  void set_##NAME( const remove_const<TYPE>::type& a ) { NAME ## _ = a; }

///
/// Macro used to declare a class property and its getter
#define DECLARE_RO_PROPERTY(NAME, TYPE)                \
private:                                               \
  TYPE NAME ## _;                                      \
public:                                                \
  const remove_const<TYPE>::type& NAME() const { return NAME ## _; }

///
/// Macro used to declare a class property and its setter
#define DECLARE_WO_PROPERTY(NAME, TYPE)                \
private:                                               \
  TYPE NAME ## _;                                      \
public:                                                \
  void set_##NAME( const TYPE& a ) { NAME ## _ = a; }

}

#endif
