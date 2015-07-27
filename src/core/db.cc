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

#include <iostream>

#include "db.hh"

namespace Db {
boost::mutex Connection::mutex;

template <>
bool Value::as<bool>() const
{
    return std::string( value_, len_ ) == "t" ? true : false;
}

template <>
std::string Value::as<std::string>() const
{
    return std::string( value_, len_ );
}

template <>
Tempus::Time Value::as<Tempus::Time>() const
{
    Tempus::Time t;
    int h, m, s;
    sscanf( value_, "%d:%d:%d", &h, &m, &s );
    t.n_secs = s + m * 60 + h * 3600;
    return t;
}

template <>
long long Value::as<long long>() const
{
    long long v;
    sscanf( value_, "%lld", &v );
    return v;
}
template <>
int Value::as<int>() const
{
    int v;
    sscanf( value_, "%d", &v );
    return v;
}
template <>
float Value::as<float>() const
{
    float v;
    sscanf( value_, "%f", &v );
    return v;
}
template <>
double Value::as<double>() const
{
    double v;
    sscanf( value_, "%lf", &v );
    return v;
}

template <>
std::vector<Tempus::db_id_t> Value::as< std::vector<Tempus::db_id_t> >() const
{
    std::vector<Tempus::db_id_t> res;
    std::string array_str( value_ );
    // trim '{}'
    std::istringstream array_sub( array_str.substr( 1, array_str.size()-2 ) );

    // parse each array element
    std::string n_str;
    while ( std::getline( array_sub, n_str, ',' ) ) {
        std::istringstream n_stream( n_str );
        Tempus::db_id_t id;
        n_stream >> id;
        res.push_back(id);
    }
    return res;
}

ResultIterator::ResultIterator( PGconn* conn ) : conn_( conn ), res_(0) {
    BOOST_ASSERT( conn_ );
    this->operator++(0);
}

ResultIterator::ResultIterator() : conn_(0), res_(0)
{
}

ResultIterator::ResultIterator( ResultIterator&& other )
{
    conn_ = other.conn_;
    res_ = other.res_;
    other.res_ = 0;
}

ResultIterator& ResultIterator::operator=( ResultIterator&& other )
{
    if ( res_ && res_ != other.res_ )
    {
        PQclear( res_ );
    }
    conn_ = other.conn_;
    res_ = other.res_;
    other.res_ = 0;
    return *this;
}

bool ResultIterator::operator==( const ResultIterator& other ) const
{
    return res_ == other.res_;
}

bool ResultIterator::operator!=( const ResultIterator& other ) const
{
    return res_ != other.res_;
}

ResultIterator::~ResultIterator()
{
    if ( res_ )
    {
        PQclear( res_ );
    }
}

RowValue ResultIterator::operator* () const
{
    return RowValue( res_, 0 );
}

void ResultIterator::operator++(int)
{
    if (res_)
    {
        PQclear( res_ );
        res_ = 0;
    }
    res_ = PQgetResult( conn_ );
    if ( PQresultStatus(res_) != PGRES_SINGLE_TUPLE )
    {
        if ( res_ ) PQclear( res_ );
        res_ = PQgetResult(conn_); // to achieve the command
        if ( res_ ) PQclear( res_ );        
        res_ = 0;
    }
}

ResultIterator Connection::exec_it( const std::string& query ) throw (std::runtime_error)
{
    int ret;
    ret = PQsendQuery( conn_, query.c_str() );
    if ( !ret )
    {
        std::string err = std::string("sendQuery: ") + PQerrorMessage( conn_ );
        throw std::runtime_error( err.c_str() );
    }
    ret = PQsetSingleRowMode( conn_ );
    if ( !ret )
    {
        std::string err = std::string("setSingleMode: ") + PQerrorMessage( conn_ );
        throw std::runtime_error( err.c_str() );
    }

    return ResultIterator( conn_ );
}

}


