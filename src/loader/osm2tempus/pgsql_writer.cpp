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
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 0 ) = points[i].lon;
        *reinterpret_cast<double*>( &ewkb[13] + 16*i + 8 ) = points[i].lat;
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
        std::cout << std::setprecision(8) << points[i].lon << " " << points[i].lat;
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
    std::cout << "commit;" << std::endl;
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

///
/// A binary COPY-based SQL writer
SQLBinaryCopyWriter::SQLBinaryCopyWriter( const std::string& db_params ) : section_id( 0 ), db( db_params )
{
    db.exec( "drop table if exists edges" );
    db.exec( "create unlogged table edges(id serial, node_from bigint, node_to bigint, tags hstore, geom geometry(linestring, 4326))" );
    db.exec( "copy edges(node_from, node_to, tags, geom) from stdin with (format binary)" );
    const char header[] = "PGCOPY\n\377\r\n\0\0\0\0\0\0\0";
    db.put_copy_data( header, 19 );
}

void SQLBinaryCopyWriter::write_section( uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags )
{
    const size_t l = 2 /*size*/ + (4+8)*2;
    char data[ l ];
    // number of fields
    uint16_t size = htons( 4 );
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

    db.put_copy_data( tags_to_binary( tags ) );

    // geometry
    std::string geom_wkb = linestring_to_ewkb( points );
    uint32_t geom_size = htonl( geom_wkb.length() );
    db.put_copy_data( reinterpret_cast<char*>( &geom_size ), 4 );
    db.put_copy_data( geom_wkb );
}

SQLBinaryCopyWriter::~SQLBinaryCopyWriter()
{
    std::string end = "\xff\xff";
    db.put_copy_data( end );
    db.put_copy_end();
}
