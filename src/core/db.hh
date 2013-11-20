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

#include <libpq-fe.h>

#include <string>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#ifdef DB_TIMING
#   include <boost/timer/timer.hpp>
#endif

#include "common.hh"

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
    T as() {
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
bool Value::as<bool>();
template <>
std::string Value::as<std::string>();
template <>
Tempus::Time Value::as<Tempus::Time>();
template <>
long long Value::as<long long>();
template <>
int Value::as<int>();
template <>
float Value::as<float>();
template <>
double Value::as<double>();

///
/// Class used to represent a row in a result.
class RowValue {
public:
    RowValue( PGresult* res, size_t nrow ) : res_( res ), nrow_( nrow ) {
    }

    ///
    /// Access to a value by column number
    Value operator [] ( size_t fn ) {
        BOOST_ASSERT( fn < ( size_t )PQnfields( res_ ) );
        return Value( PQgetvalue( res_, nrow_, fn ),
                      PQgetlength( res_, nrow_, fn ),
                      PQgetisnull( res_, nrow_, fn ) != 0 ? true : false
                    );
    }
protected:
    PGresult* res_;
    size_t nrow_;
};

///
/// Class representing result of a query
class Result {
public:
    Result( PGresult* res ) : res_( res ) {
        BOOST_ASSERT( res_ );
    }

    // default cpu ctor is ok and allowed only to be able to return by value
    // ideally (c++11) we would use the move ctor

    virtual ~Result() {
        PQclear( res_ );
    }

    ///
    /// Number of rows
    size_t size() {
        return PQntuples( res_ );
    }

    ///
    /// Number of columns
    size_t columns() {
        return PQnfields( res_ );
    }

    ///
    /// Access to a row of a result, by row number
    RowValue operator [] ( size_t idx ) {
        BOOST_ASSERT( idx < size() );
        return RowValue( res_, idx );
    }

protected:
    PGresult* res_;
private:
    Result& operator=( const Result& ); // we don't really want copies

};

///
/// Class representing connection to a database.
class Connection: boost::noncopyable {
public:
    Connection()
        : conn_( 0 ) {
    }

    Connection( const std::string& db_options )
        : conn_( 0 )
        //, db_options_(db_options) initialized by connect
    {
        assert( PQisthreadsafe() );
        connect( db_options );
    }

    void connect( const std::string& db_options ) {
#ifdef DB_TIMING
        timer_.start();
        boost::timer::nanosecond_type start = timer_.elapsed().wall;
#endif
        boost::lock_guard<boost::mutex> lock( mutex );
        conn_ = PQconnectdb( db_options.c_str() );

        if ( conn_ == NULL || PQstatus( conn_ ) != CONNECTION_OK ) {
            std::string msg = "Database connection problem: ";
            msg += PQerrorMessage( conn_ );
            PQfinish( conn_ ); // otherwise leak
            throw std::runtime_error( msg.c_str() );
        }

#ifdef DB_TIMING
        std::cout << conn_ << " " << ( timer_.elapsed().wall - start )*1.e-9 << "s in connection\n";
        timer_.stop();
#endif
    }

    virtual ~Connection() {
#ifdef DB_TIMING
        timer_.resume();
        boost::timer::nanosecond_type start = timer_.elapsed().wall;
#endif
        PQfinish( conn_ );
#ifdef DB_TIMING
        std::cout << conn_ << " " << ( timer_.elapsed().wall - start )*1.e-9 << "s in finish\n";
        timer_.stop();
        std::cout << conn_ << " total elapsed " << timer_.elapsed().wall*1.e-9 << "s\n";
#endif

    }

    ///
    /// Query execution. Returns a Db::Result. Throws a std::runtime_error on problem
    Result exec( const std::string& query ) throw ( std::runtime_error ) {
#ifdef DB_TIMING
        timer_.resume();
        boost::timer::nanosecond_type start = timer_.elapsed().wall;
#endif

        PGresult* res = PQexec( conn_, query.c_str() );
        ExecStatusType ret = PQresultStatus( res );

        if ( ( ret != PGRES_COMMAND_OK ) && ( ret != PGRES_TUPLES_OK ) ) {
            std::string msg = "Problem on database query: ";
            msg += PQresultErrorMessage( res );
            PQclear( res );
            throw std::runtime_error( msg.c_str() );
        }

#ifdef DB_TIMING
        std::cout << conn_ << " " << ( timer_.elapsed().wall - start )*1.e-9 << "s in query: " << query << "\n";
        timer_.stop();
#endif

        return res;
    }
protected:
    PGconn* conn_;
    static boost::mutex mutex;

#ifdef DB_TIMING
    boost::timer::cpu_timer timer_;
#endif
};
}

#endif
