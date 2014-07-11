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

#ifndef TEMPUS_BASE_HH
#define TEMPUS_BASE_HH

namespace Tempus
{

///
/// Type used inside the DB to store IDs.
/// O means NULL.
///
typedef unsigned long long int db_id_t;

///
/// The base class of all classes with a database ID
class Base
{
public:
    Base() : db_id_(0) {}
    explicit Base( db_id_t id ) : db_id_(id) {}
    db_id_t db_id() const { return db_id_; }
    void    set_db_id( const db_id_t& id ) { db_id_ = id; }

private:
    ///
    /// Persistant ID relative to the storage database.
    /// Common to many classes.
    db_id_t db_id_;
};


///
/// Class with a possible null value - FIXME replace by boost::optional ?
template <class T>
class OrNull
{
public:
    OrNull() : is_null_(true) {}
    OrNull( const T& v ) : is_null_(false), v_(v) {}
    bool is_null() const { return is_null_; }
    bool is_not_null() const { return ! is_null_; }
    operator T() const { return v_; }
    operator bool() const { return ! is_null(); }
    T value() const { return v_; }
    void reset() { is_null_ = true; }
private:
    bool is_null_;
    T v_;
};


}

#endif
