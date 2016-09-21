#include "pgsql_writer.h"

static std::string linestring_to_ewkb( const std::vector<Point>& points )
{
    std::string ewkb;
    ewkb.resize( 1 + 4 /*type*/ + 4 /*srid*/ + 4 /*size*/ + 8 * points.size() * 2 );
    memcpy( &ewkb[0], "\x01"
            "\x02\x00\x00\x20"  // linestring + has srid
            "\xe6\x10\x00\x00", // srid = 4326
            9
            );
    // size
    *reinterpret_cast<uint32_t*>( &ewkb[9] ) = points.size();
    for ( size_t i = 0; i < points.size(); i++ ) {
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 0 ) = points[i].lon();
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 8 ) = points[i].lat();
    }
    return ewkb;
}

static std::string binary_to_hextext( const std::string& str )
{
    std::ostringstream ostr;
    for ( char c : str ) {
        ostr << std::hex << std::setw( 2 ) << std::setfill('0') << static_cast<int>( static_cast<unsigned char>( c ) );
    }
    return ostr.str();
}

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

static std::string escape_pgquotes( std::string s )
{
    size_t pos = 0;
    while ((pos = s.find('\'', pos)) != std::string::npos) {
        s.replace(pos, 1, "''");
        pos += 2;
    }
    return s;
}


SQLWriter::SQLWriter() : section_id( 0 )
{
    //std::cout << "set client encoding to 'utf8';" << std::endl;
    std::cout << "drop table if exists edges;" << std::endl;
    std::cout << "create table edges(id serial primary key, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326));" << std::endl;
    std::cout << "begin;" << std::endl;
}
    
void SQLWriter::write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
{
    std::cout << "INSERT INTO edges (id,node_from,node_to, geom, tags) VALUES (" << ++section_id << ","
              << node_from << ","
              << node_to << ","
              << "'SRID=4326;LINESTRING(";
    for ( size_t i = 0; i < points.size(); i++ )
    {
        std::cout << std::setprecision(8) << points[i].lon() << " " << points[i].lat();
        if ( i < points.size() - 1 )
            std::cout << ",";
    }
    std::cout << ")',";
    std::cout << "hstore(array[";
    bool first = true;
    for ( const auto& p: tags ) {
        if ( !first ) {
            std::cout << ",";
        }
        first = false;
        std::cout << "'" << p.first << "','" << escape_pgquotes(p.second) << "'";
    }
    std::cout << "]));" << std::endl;
}

SQLWriter::~SQLWriter()
{
}

///
/// A basic COPY-based SQL writer
SQLCopyWriter::SQLCopyWriter( const std::string& db_params ) : section_id( 0 ), db( db_params )
{
    db.exec( "drop table if exists edges" );
    db.exec( "create unlogged table edges(id serial primary key, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326))" );
    db.exec( "copy edges(node_from, node_to, geom) from stdin with delimiter ';'" );
}
    
void SQLCopyWriter::write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& /*tags*/ )
{
    std::ostringstream data;
    data << node_from << ";" << node_to << ";" << binary_to_hextext( linestring_to_ewkb( points ) ) << std::endl;
    db.put_copy_data( data.str() );
}

SQLCopyWriter::~SQLCopyWriter()
{
    db.put_copy_end();
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
                                          const std::string& table,
                                          bool create_table,
                                          DataProfile* profile,
                                          bool keep_tags )
    : Writer( profile, keep_tags ), section_id( 0 ), db( db_params ), schema_( schema ), table_( table ), create_table_( create_table )
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
    if ( create_table ) {
        db.exec( "create extension if not exists postgis" );
        db.exec( "create extension if not exists hstore" );
        db.exec( "create schema if not exists " + schema );
        db.exec( "drop table if exists " + schema + "." + table );
        db.exec( "create unlogged table " + schema + "." + table +
                 "(id serial, node_from bigint, node_to bigint, geom geometry(linestring, 4326)" +
                 additional_columns_with_type +
                 additional_tags_with_type +
                 ")" );
    }
    db.exec( "copy " + schema + "." + table + "(node_from, node_to, geom" + additional_columns + additional_tags + ") from stdin with (format binary)" );
    const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0\0\0";
    db.put_copy_data( header, 19 );
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
        data_.append( 4 + 8, 0 );
        char *p = &data_.back() - 12 + 1;
        uint32_t size = htonl( 8 );
        uint32_t vv = htonl( *reinterpret_cast<uint32_t*>(&f) );
        memcpy( p, &size, 4 ); p+= 4;
        memcpy( p, &vv, 4 );
    }
    void operator()( double d )
    {
        data_.append( 4 + 8, 0 );
        char *p = &data_.back() - 12 + 1;
        uint32_t size = htonl( 8 );
        uint64_t vv = htobe64( *reinterpret_cast<uint64_t*>(&d) );
        memcpy( p, &size, 4 ); p+= 4;
        memcpy( p, &vv, 8 );
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

void SQLBinaryCopyWriter::write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
{
    const size_t l = 2 /*size*/ + (4+8)*2;
    char data[ l ];
    // number of fields
    uint16_t size = htons( 3  + ( data_profile_ ? data_profile_->n_columns() : 0 ) + ( keep_tags_ ? 1 : 0 ) );
    // size of a uint64
    uint32_t size2 = htonl( 8 );
    char *p = &data[0];
    memcpy( p, &size, 2 ); p += 2;
    memcpy( p, &size2, 4 ); p+= 4;
    node_from = htobe64( node_from );
    node_to = htobe64( node_to );
    memcpy( p, &node_from, 8 ); p+= 8;
    memcpy( p, &size2, 4 ); p+= 4;
    memcpy( p, &node_to, 8 ); p+= 8;
    db.put_copy_data( data, l );

    // geometry
    std::string geom_wkb = linestring_to_ewkb( points );
    uint32_t geom_size = htonl( geom_wkb.length() );
    db.put_copy_data( reinterpret_cast<char*>( &geom_size ), 4 );
    db.put_copy_data( geom_wkb );

    if ( data_profile_ ) {
        std::string add_data;
        pg_data_visitor vis( add_data );
        for ( const auto& v : data_profile_->section_additional_values( node_from, node_to, points, tags ) ) {
            boost::apply_visitor( vis, v );
        }
        db.put_copy_data( add_data );
    }

    if ( keep_tags_ )
        db.put_copy_data( tags_to_binary( tags ) );
}

SQLBinaryCopyWriter::~SQLBinaryCopyWriter()
{
    db.put_copy_data( "\xff\xff" );
    db.put_copy_end();

    if ( create_table_ ) {
        std::cout << "creating index" << std::endl;
        // create index
        db.exec( "create unique index on " + schema_ + "." + table_ + "(id)" );
        db.exec( "alter table " + schema_ + "." + table_ + " add primary key using index road_section_id_idx" );
    }
}
