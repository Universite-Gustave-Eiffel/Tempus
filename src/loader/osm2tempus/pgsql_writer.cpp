/**
 *   Copyright (C) 2016 Oslandia <infos@oslandia.com>
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

#include "pgsql_writer.h"
#include "geom.h"

static std::string tags_to_binary( const osm_pbf::Tags& tags )
{
    uint32_t size1, size2, wsize;
    // total size;
    uint32_t tsize = 4;
    for ( const auto& tag: tags ) {
        tsize += tag.first.length() + tag.second.length() + 8;
    }
    
    std::string out;
    out.resize( tsize + 4 );
    char *p = &out[0];

    // total size
    wsize = htonl( tsize );
    memcpy( p, &wsize, 4 ); p += 4;
    
    // number of tags
    wsize = htonl( tags.size() );
    memcpy( p, &wsize, 4 ); p += 4;
    for ( const auto& tag : tags )
    {
        size1 = tag.first.length();
        size2 = tag.second.length();
        wsize = htonl( size1 );
        memcpy( p, &wsize, 4 ); p += 4;
        memcpy( p, tag.first.data(), size1 ); p += size1;
        wsize = htonl( size2 );
        memcpy( p, &wsize, 4 ); p += 4;
        memcpy( p, tag.second.data(), size2 ); p += size2;
    }
    return out;
}

// DataProfile::DataType -> pg type
static const std::string pg_types[] = { "boolean",
                                        "smallint",
                                        "smallint",
                                        "smallint",
                                        "smallint",
                                        "integer",
                                        "integer",
                                        "bigint",
                                        "bigint",
                                        "real",
                                        "double precision",
                                        "text" };

///
/// A binary COPY-based SQL writer
SQLBinaryCopyWriter::SQLBinaryCopyWriter( const std::string& db_params,
                                          const std::string& schema,
                                          const std::string& sections_table,
                                          const std::string& nodes_table,
                                          const std::string& restrictions_table,
                                          bool create_table,
                                          DataProfile* profile,
                                          bool keep_tags )
: Writer( profile, keep_tags ),
  db( db_params ),
  schema_( schema ),
  sections_table_( sections_table ),
  nodes_table_( nodes_table ),
  restrictions_table_( restrictions_table ),
  create_table_( create_table )
{
    if ( create_table ) {
        db.exec( "create extension if not exists postgis" );
        if ( keep_tags_ ) {
            db.exec( "create extension if not exists hstore" );
        }
        db.exec( "create schema if not exists " + schema );
    }

    // start transaction and disable constraints until end of transaction
    db.exec( "begin; set constraints all deferred;");
}

SQLBinaryCopyWriter::~SQLBinaryCopyWriter()
{
    db.exec( "commit");
}

void SQLBinaryCopyWriter::begin_sections()
{
    std::string additional_columns, additional_columns_with_type;
    if ( data_profile_ ) {
        for ( const auto& c : data_profile_->columns() ) {
            additional_columns += ", " + c.name;
            additional_columns_with_type += ", " + c.name + " " + pg_types[c.type];
        }
    }
    std::string additional_tags, additional_tags_with_type;
    if ( keep_tags_ ) {
        additional_tags = ", tags";
        additional_tags_with_type = ", tags hstore";
    }
    if ( create_table_ ) {
        db.exec( "drop table if exists " + schema_ + "." + sections_table_ );
        db.exec( "create unlogged table " + schema_ + "." + sections_table_ +
                 "(id bigint, node_from bigint, node_to bigint, geom geometry(linestringz, 4326)" +
                 additional_columns_with_type +
                 additional_tags_with_type +
                 ")" );
    }
    db.exec( "copy " + schema_ + "." + sections_table_ + "(id, node_from, node_to, geom" + additional_columns + additional_tags + ") from stdin with (format binary)" );
    const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0\0\0";
    db.put_copy_data( header, 19 );
    n_sections_ = 0;
}

void SQLBinaryCopyWriter::begin_nodes()
{
    if ( create_table_ ) {
        db.exec( "drop table if exists " + schema_ + "." + nodes_table_ );
        db.exec( "create unlogged table " + schema_ + "." + nodes_table_ +
                 "(id bigint, geom geometry(pointz, 4326))" );
    }
    db.exec( "copy " + schema_ + "." + nodes_table_ + "(id, geom) from stdin with (format binary)" );
    const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0\0\0";
    db.put_copy_data( header, 19 );
}

void SQLBinaryCopyWriter::begin_restrictions()
{
    if ( create_table_ ) {
        db.exec( "drop table if exists " + schema_ + "." + restrictions_table_ );
        db.exec( "create unlogged table " + schema_ + "." + restrictions_table_ +
                 "(id bigint, sections bigint[])" );
    }
    db.exec( "copy " + schema_ + "." + restrictions_table_ + "(id, sections) from stdin with (format binary)" );
    const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0\0\0";
    db.put_copy_data( header, 19 );
}

void SQLBinaryCopyWriter::write_restriction( uint64_t id, const std::vector<uint64_t>& section_ids )
{
    if ( section_ids.size() != 2 )
        return;
    
    const size_t l = 2 /*size*/ + (4+8) * 3 + 6*4;
    char data[ l ] = "\x00\x02" // number of fields
        "\x00\x00\x00\x08" // size of uint64
        "\x00\x00\x00\x00"
        "\x00\x00\x00\x00"
        "\x00\x00\x00\x2c" // size of array = 44 = 4*5 + 2*12
        "\x00\x00\x00\x01" // ndim
        "\x00\x00\x00\x00" // flags
        "\x00\x00\x00\x14" // bigint
        "\x00\x00\x00\x02" // number of elements
        "\x00\x00\x00\x01" // lower bound
        ;
    
    // restriction id
    char *p = &data[6];
    id = htobe64( id );
    memcpy( p, &id, 8 );

    // array of section id
    p = &data[38];
    for ( int i = 0; i < 2; i++ ) {
        // size
        uint32_t size = htonl( 8 );
        memcpy( p, &size, 4 ); p+= 4;
        // value
        uint64_t v = htobe64( section_ids[i] );
        memcpy( p, &v, 8 ); p+= 8;
    }

    db.put_copy_data( data, l );
}

void SQLBinaryCopyWriter::end_restrictions()
{
    db.put_copy_data( "\xff\xff" );
    db.put_copy_end();
}


void SQLBinaryCopyWriter::write_node( uint64_t node_id, float lat, float lon )
{
    const size_t l = 2 /*size*/ + (4+8);
    char data[ l ];
    // number of fields
    uint16_t size = htons( 2 );
    // size of a uint64
    uint32_t size2 = htonl( 8 );
    char *p = &data[0];
    memcpy( p, &size, 2 ); p += 2;

    node_id = htobe64( node_id );
    memcpy( p, &size2, 4 ); p+= 4;
    memcpy( p, &node_id, 8 ); p+= 8;

    db.put_copy_data( data, l );

    // geometry
    std::string geom_wkb = point_to_ewkb_with_z( lat, lon );
    uint32_t geom_size = htonl( geom_wkb.length() );
    db.put_copy_data( reinterpret_cast<char*>( &geom_size ), 4 );
    db.put_copy_data( geom_wkb );
}

void SQLBinaryCopyWriter::end_nodes()
{
    db.put_copy_data( "\xff\xff" );
    db.put_copy_end();

    if ( create_table_ ) {
        std::cout << "creating index" << std::endl;
        // create index
        db.exec( "create unique index on " + schema_ + "." + nodes_table_ + "(id)" );
        db.exec( "alter table " + schema_ + "." + nodes_table_ + " add primary key using index " + nodes_table_ + "_id_idx" );
    }
}

struct pg_data_visitor : public boost::static_visitor<void>
{
    pg_data_visitor( std::string& data ) : data_(data) {}
    void operator()( bool ) {}
    void operator()( int8_t v ) { basic_op( v ); }
    void operator()( uint8_t v ) { basic_op( v ); }
    void operator()( int16_t v ) { basic_op( v ); }
    void operator()( uint16_t v ) { basic_op( v ); }
    void operator()( int32_t v ) { basic_op( v ); }
    void operator()( uint32_t v ) { basic_op( v ); }
    void operator()( int64_t v ) { basic_op( v ); }
    void operator()( uint64_t v ) { basic_op( v ); }
    void operator()( float f )
    {
        data_.append( 4 + sizeof(float), 0 );
        char *p = &data_.back() - 4 - sizeof(float) + 1;
        uint32_t size = htonl( sizeof(float) );
        uint32_t vv;
        memcpy( &vv, &f, sizeof(float) );
        vv = htonl( vv );
        memcpy( p, &size, 4 ); p+= 4;
        memcpy( p, &vv, sizeof(float) );
    }
    void operator()( double d )
    {
        data_.append( 4 + sizeof(double), 0 );
        char *p = &data_.back() - 4 - sizeof(double) + 1;
        uint32_t size = htonl( sizeof(double) );
        uint64_t vv;
        memcpy( &vv, &d, sizeof(double) );
        vv = htobe64( vv );
        memcpy( p, &size, 4 ); p+= 4;
        memcpy( p, &vv, sizeof(double) );
    }
    void operator()( const std::string& txt )
    {
        data_.append( 4 + txt.size(), 0 );
        char *p = &data_.back() - 4 - txt.size() + 1;
        uint32_t size = htonl( txt.size() );
        memcpy( p, &size, 4 ); p+= 4;
        memcpy( p, txt.data(), txt.size() );
    }
private:
    std::string& data_;
    template <typename T>
    void basic_op( T v )
    {
        data_.append( 4 + sizeof(T), 0 );
        char *p = &data_.back() - 4 - sizeof(T) + 1;
        uint32_t size = htonl( sizeof(T) );
        T vv = reverse_bytes( v );
        memcpy( p, &size, 4 ); p += 4;
        memcpy( p, &vv, sizeof(T) );
    }
    int8_t reverse_bytes( int8_t v ) const { return v; }
    uint8_t reverse_bytes( uint8_t v ) const { return v; }
    int16_t reverse_bytes( int16_t v ) const { return htons(v); }
    uint16_t reverse_bytes( uint16_t v ) const { return htons(v); }
    int32_t reverse_bytes( int32_t v ) const { return htonl(v); }
    uint32_t reverse_bytes( uint32_t v ) const { return htonl(v); }
    int64_t reverse_bytes( int64_t v ) const { return htobe64(v); }
    uint64_t reverse_bytes( uint64_t v ) const { return htobe64(v); }
};

void SQLBinaryCopyWriter::write_section( uint64_t way_id, uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
{
    const size_t l = 2 /*size*/ + (4+8)*3;
    char data[ l ];
    // number of fields
    uint16_t size = htons( 4  + ( data_profile_ ? data_profile_->n_columns() : 0 ) + ( keep_tags_ ? 1 : 0 ) );
    // size of a uint64
    uint32_t size2 = htonl( 8 );
    char *p = &data[0];
    memcpy( p, &size, 2 ); p += 2;

    section_id = htobe64( section_id );
    memcpy( p, &size2, 4 ); p+= 4;
    memcpy( p, &section_id, 8 ); p+= 8;

    node_from = htobe64( node_from );
    memcpy( p, &size2, 4 ); p+= 4;
    memcpy( p, &node_from, 8 ); p+= 8;

    node_to = htobe64( node_to );
    memcpy( p, &size2, 4 ); p+= 4;
    memcpy( p, &node_to, 8 ); p+= 8;

    db.put_copy_data( data, l );

    // geometry
    std::string geom_wkb = linestring_to_ewkb_with_z( points );
    uint32_t geom_size = htonl( geom_wkb.length() );
    db.put_copy_data( reinterpret_cast<char*>( &geom_size ), 4 );
    db.put_copy_data( geom_wkb );

    if ( data_profile_ ) {
        std::string add_data;
        pg_data_visitor vis( add_data );
        for ( const auto& v : data_profile_->section_additional_values( way_id, section_id, node_from, node_to, points, tags ) ) {
            boost::apply_visitor( vis, v );
        }
        db.put_copy_data( add_data );
    }

    if ( keep_tags_ )
        db.put_copy_data( tags_to_binary( tags ) );

    n_sections_++;
}

void SQLBinaryCopyWriter::end_sections()
{
    db.put_copy_data( "\xff\xff" );
    db.put_copy_end();

    if ( create_table_ ) {
        std::cout << "creating index" << std::endl;
        // create index
        db.exec( "create unique index on " + schema_ + "." + sections_table_ + "(id)" );
        db.exec( "alter table " + schema_ + "." + sections_table_ + " add primary key using index " + sections_table_ + "_id_idx" );
    }
    std::cout << std::dec << n_sections_ << " sections created" << std::endl;
}
