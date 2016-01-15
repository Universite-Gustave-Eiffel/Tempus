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

#ifndef TEMPUS_DB_HH
#define TEMPUS_DB_HH

/**
   Database access is modeled by means of the following classes, inspired by pqxx:
   * A Db::Connection objet represents a connection to a database. It is a lightweighted objet that is reference-counted and thus can be copied safely.
   * A Db::Result objet represents result of a query. It is a lightweighted objet that is reference-counted and thus can be copied safely.
   * A Db::RowValue object represents a row of a result and is obtained by Db::Result::operator[]
   * A Db::Value object represent a basic value. It is obtained by Db::RowValue::operator[]. It has templated conversion operators for common data types.

   These classes throw std::runtime_error on problem.
 */

#include <string>
#include <stdexcept>

#ifdef _WIN32
#pragma warning(push, 0)
#endif
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#ifdef DB_TIMING
#   include <boost/timer/timer.hpp>
#endif

#include "common.hh"

struct pg_result;
struct pg_conn;

namespace Db {
///
/// Class representing an atomic value stored in a database.
class Value {
public:
    Value( const char* value, size_t len, bool isnull ) : value_( value ), len_( len ), isnull_( isnull ) {
    }

    ///
    /// This is the generic conversion operator.
    /// It calls stringstream conversion operators (slow!).
    /// Specialization can be introduced, or via a specialization of the stringstream::operator>>()
    template <class T>
    T as() const {
        T obj;
        std::istringstream istr( value_ );
        istr >> obj;
        return obj;
    }

    ///
    /// Conversion operator. Does nothing if the underlying object is null (which is a special value in a database)
    template <class T>
    void operator >> ( T& obj ) {
        if ( !isnull_ ) {
            obj = as<T>();
        }
    }

    ///
    /// Casting operator
    template <class T>
    operator T () {
        return as<T>();
    }

    ///
    /// Tests if the underlying object is null
    bool is_null() {
        return isnull_;
    }
protected:
    const char* value_;
    size_t len_;
    bool isnull_;
};

//
// List of conversion specializations
template <>
bool Value::as<bool>() const;
template <>
std::string Value::as<std::string>() const;
template <>
Tempus::Time Value::as<Tempus::Time>() const;
template <>
long long Value::as<long long>() const;
template <>
int Value::as<int>() const;
template <>
float Value::as<float>() const;
template <>
double Value::as<double>() const;
template <>
std::vector<Tempus::db_id_t> Value::as<std::vector<Tempus::db_id_t> >() const;

///
/// Class used to represent a row in a result.
class RowValue {
public:
    RowValue( pg_result* res, size_t nrow );

    ///
    /// Access to a value by column number
    Value operator [] ( size_t fn );

protected:
    pg_result* res_;
    size_t nrow_;
};

///
/// Class representing result of a query
class Result {
public:
    Result( pg_result* res );

    // Result is only movable
    Result( Result&& other );
    Result& operator=( Result&& other );

    ~Result();

    ///
    /// Number of rows
    size_t size() const;

    ///
    /// Number of columns
    size_t columns() const;

    ///
    /// Access to a row of a result, by row number
    RowValue operator [] ( size_t idx ) const;

protected:
    pg_result* res_;
private:
    // non copyable
    Result( const Result& other );
    Result& operator=( const Result& );
};

///
/// Class representing a result iterator
class ResultIterator {
public:
    ResultIterator();
    ResultIterator( pg_conn* conn );
    ~ResultIterator();

    // a result iterator is moveable
    ResultIterator( ResultIterator&& other );
    ResultIterator& operator=( ResultIterator&& other );

    bool operator==( const ResultIterator& other ) const;
    bool operator!=( const ResultIterator& other ) const;
    RowValue operator* () const;
    void operator++(int);
private:
    // non copyable
    ResultIterator( const ResultIterator& other );
    ResultIterator& operator=( const ResultIterator& other );

    pg_conn* conn_;
    pg_result* res_;
};

///
/// Class representing connection to a database.
class Connection: boost::noncopyable {
public:
    Connection();
    Connection( const std::string& db_options );
    ~Connection();

    void connect( const std::string& db_options );

    ///
    /// Query execution. Returns a Db::Result. Throws a std::runtime_error on problem
    Result exec( const std::string& query ) throw ( std::runtime_error );

    ///
    /// Query execution using a cursor
    ResultIterator exec_it( const std::string& query ) throw ( std::runtime_error );

protected:
    pg_conn* conn_;
    static boost::mutex mutex;

#ifdef DB_TIMING
    boost::timer::cpu_timer timer_;
#endif
};
}

#endif
