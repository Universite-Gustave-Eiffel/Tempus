// osm2shp
//
// a fast and almost flexible converter from OSM XML to shapefiles
//
// this program was inspired by Jochen Topf's "osmexport" ruby script.
// the file you're looking at contains the code that you are unlikely
// to change. at one point the file osm2shp.config is included, and
// in there you will find three functions that you have to modify,
// defining what shapefiles you want and what should be written to
// them. this is roughly equivalent to the "rules file" you have with
// osmexport, with the minor difference that you will get a segfault
// if you do something wrong there.
//
// Originally written by Frederik Ramm <frederik@remote.org>, public domain

// This was originally C, but has been converted to C++ to avoid dependency
// on glib for hash

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


#include <stdio.h>
#include <shapefil.h>
#ifdef WIN32
#   define strcasecmp _stricmp
#else
#   include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>

#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>

#define MAX_NODES_PER_WAY 5000
#define MAX_DBF_FIELDS 16
#define MAX_SHAPEFILES 16

#define TO_INT64(x) strtoll((x), NULL, 10)

struct Coord { // struct to allow cpy
    double& operator[]( int i ) {
        return coord[i];
    }
    const double& operator[]( int i ) const {
        return coord[i];
    }
private:
    double coord[2];
};

struct RelationMember {
    std::string role;
    enum Type {
        MEMBER_NODE,
        MEMBER_WAY,
        MEMBER_RELATION
    };
    Type type;
    int64_t ref;
};

std::map<int64_t,Coord> node_storage;
std::map<std::string,std::string> current_tags;
int64_t current_nodes[MAX_NODES_PER_WAY];
int current_node_count = 0;
bool too_many_nodes_warning_issued = false;
int64_t current_id;
char* current_timestamp;
Coord current_latlon;
std::vector<RelationMember> current_members;

std::string outdir = ".";

struct sf {
    SHPHandle shph;
    DBFHandle dbfh;
    int num_fields;
    int fieldtype[MAX_DBF_FIELDS];
    int fieldwidth[MAX_DBF_FIELDS];
} shapefiles[MAX_SHAPEFILES];

#ifndef LATIN1
void truncate_utf8( char* string, int maxbytes )
{
    int bytes = strlen( string );

    if ( bytes <= maxbytes ) {
        return;
    }

    *( string + maxbytes ) = 0;

    while( !xmlCheckUTF8( ( const unsigned char* )string ) && maxbytes > 0 ) {
        maxbytes--;
        *( string + maxbytes ) = 0;
    }
}
#endif

void die( const char* fmt, ... )
{
    char* cpy;
    va_list ap;
    cpy = ( char* ) malloc( strlen( fmt ) + 2 );
    sprintf( cpy, "%s\n", fmt );
    va_start( ap, fmt );
    vfprintf( stderr, cpy, ap );
    va_end( ap );
    exit( 1 );
}
void warn( const char* fmt, ... )
{
    char* cpy;
    va_list ap;
    cpy = ( char* ) malloc( strlen( fmt ) + 2 );
    sprintf( cpy, "%s\n", fmt );
    va_start( ap, fmt );
    vfprintf( stderr, cpy, ap );
    va_end( ap );
    free( cpy );
}


void shapefile_new( int slot, int filetype, const char* filename, int num_fields, ... )
{
    va_list ap;
    int i;
    struct sf* shape;
    assert( slot < MAX_SHAPEFILES );
    shape = &( shapefiles[slot] );
    const std::string filepath = outdir+"/"+filename;
    if ( filetype != SHPT_NULL ) {
        shape->shph = SHPCreate( filepath.c_str(), filetype );
        if ( shape->shph==0 ) {
            die( "cannot create shapefile '%s'", filepath.c_str() );
        }
    }
    else {
        shape->shph = 0;
    }

    shape->dbfh = DBFCreate( filepath.c_str() );
    if ( shape->dbfh==0 ) {
        die( "cannot create dbf file '%s'", filepath.c_str() );
    }

    shape->num_fields = num_fields;
    va_start( ap, num_fields );

    for ( i=0; i<num_fields; i++ ) {
        char* name = va_arg( ap, char* );
        int type = va_arg( ap, int );
        int width = 0;
        int decimals = 0;

        if ( type == FTString || type == FTDouble || type == FTInteger ) {
            width = va_arg( ap, int );
        }

        if ( type == FTDouble ) {
            decimals = va_arg( ap, int );
        }

        if ( DBFAddField( shape->dbfh, name, DBFFieldType( type ), width, decimals ) == -1 ) {
            die( "Addition of field '%s' to '%s' failed", name, filename );
        }

        shape->fieldtype[i] = type;
        shape->fieldwidth[i] = width;
    }

    va_end( ap );
}

void shapefile_add_dbf( int slot, int entity, bool, va_list ap )
{
    struct sf* shape = &( shapefiles[slot] );
    int i;

    for ( i=0; i<shape->num_fields; i++ ) {
        switch ( shape->fieldtype[i] ) {
        case FTString: {
            char* in = va_arg( ap, char* );

            if ( !in ) {
                DBFWriteNULLAttribute( shape->dbfh, entity, i );
                break;
            }

#ifdef LATIN1
            char buffer[256];
            int outlen = 256;
            int inlen = strlen( in );
            outlen = UTF8Toisolat1( buffer, &outlen, in, &inlen );

            if ( outlen >= 0 ) {
                buffer[outlen] = 0;
                DBFWriteStringAttribute( shape->dbfh, entity, i, buffer );
            }
            else {
                warn( "UTF to ISO conversion failed for tag value '%s' of %s #%lld",
                      in, way ? "way" : "node", current_id );
            }

#else
            truncate_utf8( in, shape->fieldwidth[i] );
            DBFWriteStringAttribute( shape->dbfh, entity, i, in );
#endif
        }
        break;
        case FTInteger:
            DBFWriteIntegerAttribute( shape->dbfh, entity, i, va_arg( ap, int ) );
            break;
        case FTDouble:
            DBFWriteDoubleAttribute( shape->dbfh, entity, i, va_arg( ap, double ) );
            break;
        }
    }
}

void shapefile_add_attributes( int slot, ... )
{
    struct sf* shape = &( shapefiles[slot] );
    va_list ap;
    int nbRecord;
    va_start( ap, slot );
    nbRecord = DBFGetRecordCount( shape->dbfh );
    shapefile_add_dbf( slot, nbRecord, false, ap );
}

void shapefile_add_node( int slot, ... )
{
    struct sf* shape = &( shapefiles[slot] );
    SHPObject* o;
    va_list ap;
    int entity;
    va_start( ap, slot );
    o = SHPCreateSimpleObject( SHPT_POINT, 1, &current_latlon[1], &current_latlon[0], NULL );
    entity = SHPWriteObject( shape->shph, -1, o );
    SHPDestroyObject( o );
    shapefile_add_dbf( slot, entity, false, ap );
}

void shapefile_add_way( int slot, ... )
{
    struct sf* shape = &( shapefiles[slot] );
    assert( shape );
    double lat[MAX_NODES_PER_WAY];
    double lon[MAX_NODES_PER_WAY];
    int i;
    SHPObject* o;
    va_list ap;
    int entity;
    int j = 0;
    va_start( ap, slot );

    if ( current_node_count < 2 ) {
        return;
    }

    for ( i=0; i<current_node_count; i++ ) {
        std::map<int64_t,Coord>::const_iterator found = node_storage.find( *( current_nodes+i ) );

        if ( found != node_storage.end() ) {
            lat[j]   = found->second[0];
            lon[j++] = found->second[1];
        }
        else {
            warn( "way #%lld references undefined node #%lld", current_id, *( current_nodes+i ) );
        }
    }

    o = SHPCreateSimpleObject( SHPT_ARC, j, lon, lat, NULL );
    entity = SHPWriteObject( shape->shph, -1, o );
    SHPDestroyObject( o );
    shapefile_add_dbf( slot, entity, true, ap );
}

void shapefile_add_polygon( int slot, ... )
{
    struct sf* shape = &( shapefiles[slot] );
    double lat[MAX_NODES_PER_WAY];
    double lon[MAX_NODES_PER_WAY];
    int i;
    SHPObject* o;
    va_list ap;
    int entity;
    va_start( ap, slot );

    if ( current_node_count < 3 ) {
        return;
    }

    for ( i=0; i<current_node_count; i++ ) {
        std::map<int64_t,Coord>::const_iterator found = node_storage.find( *( current_nodes+i ) );
        lat[i] = found->second[0];
        lon[i] = found->second[1];
    }

    if ( current_nodes[0] != current_nodes[current_node_count-1] ) {
        lat[i] = *current_nodes;
        lon[i] = *( current_nodes+1 );
    }

    o = SHPCreateSimpleObject( SHPT_POLYGON, current_node_count, lon, lat, NULL );
    entity = SHPWriteObject( shape->shph, -1, o );
    SHPDestroyObject( o );
    shapefile_add_dbf( slot, entity, true, ap );
}

int extract_boolean_tag( const char* name, int def )
{
    std::map<std::string,std::string>::const_iterator found = current_tags.find( name );

    if ( found == current_tags.end() ) {
        return def;
    }

    const char* v = found->second.c_str();

    if ( strcasecmp( v, "yes" ) && strcasecmp( v, "true" ) && strcmp( v, "1" ) ) {
        return 0;
    }

    return 1;
}

int extract_integer_tag( const char* name, int def )
{
    std::map<std::string,std::string>::const_iterator found = current_tags.find( name );

    if ( found == current_tags.end() ) {
        return def;
    }

    return( atoi( found->second.c_str() ) );
}

#include "tempus.config"

void save_osm_node()
{
    node_storage[current_id] =  current_latlon;
}

void open_element( xmlTextReaderPtr reader, const xmlChar* name )
{
    xmlChar* xid, *xlat, *xlon, *xk, *xv, *xts;

    if ( xmlStrEqual( name, ( const xmlChar* )"node" ) ) {
        xid  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"id" );
        xts  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"timestamp" );
        xlon = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"lon" );
        xlat = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"lat" );
        assert( xid );
        assert( xlon );
        assert( xlat );
        current_id = TO_INT64( ( char* )xid );
        current_timestamp = ( char* ) xts;
        current_latlon[0] = strtod( ( char* )xlat, NULL );
        current_latlon[1] = strtod( ( char* )xlon, NULL );
        xmlFree( xid );
        xmlFree( xlon );
        xmlFree( xlat );
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"tag" ) ) {
        xk = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"k" );
        assert( xk );
        xv = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"v" );
        assert( xv );
        current_tags.insert( std::make_pair( (const char*)xk, (const char*)xv ) );
        xmlFree( xv );
        xmlFree( xk );
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"way" ) ) {
        xid  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"id" );
        xts  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"timestamp" );
        assert( xid );
        current_id = TO_INT64( ( char* )xid );
        current_timestamp = ( char* ) xts;
        xmlFree( xid );
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"relation" ) ) {
        xid  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"id" );
        assert( xid );
        current_id = TO_INT64( ( char* )xid );
        xmlFree( xid );
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"member" ) ) {
        xmlChar *xtype, *xref, *xrole;
        xtype  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"type" );
        xref  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"ref" );
        xrole  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"role" );

        RelationMember m;
        m.role = (const char*)xrole;
        std::string typestr = (const char*)xtype;
        if ( typestr == "node" ) {
            m.type = RelationMember::MEMBER_NODE;
        }
        else if ( typestr == "way" ) {
            m.type = RelationMember::MEMBER_WAY;
        }
        else if ( typestr == "relation" ) {
            m.type = RelationMember::MEMBER_RELATION;
        }
        m.ref = TO_INT64( ( char* )xref );
        current_members.push_back( m );
        xmlFree( xtype );
        xmlFree( xref );
        xmlFree( xrole );
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"nd" ) ) {
        xid  = xmlTextReaderGetAttribute( reader, ( const xmlChar* )"ref" );
        assert( xid );

        if ( current_node_count == MAX_NODES_PER_WAY ) {
            if ( !too_many_nodes_warning_issued ) {
                warn( "too many nodes in way #%lld", current_id );
            }

            too_many_nodes_warning_issued = true;
        }
        else if ( current_node_count < MAX_NODES_PER_WAY ) {
            current_nodes[current_node_count++] = TO_INT64( ( const char* )xid );
        }

        xmlFree( xid );
    }
}

void close_element( const xmlChar* name )
{
    if ( xmlStrEqual( name, ( const xmlChar* )"node" ) ) {
        save_osm_node();
        process_osm_node();
        current_tags.clear();

        if ( current_timestamp ) {
            xmlFree( current_timestamp );
            current_timestamp = NULL;
        }
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"way" ) ) {
        process_osm_way();
        current_node_count = 0;
        too_many_nodes_warning_issued = false;
        current_tags.clear();

        if ( current_timestamp ) {
            xmlFree( current_timestamp );
            current_timestamp = NULL;
        }
    }
    else if ( xmlStrEqual( name, ( const xmlChar* )"relation" ) ) {
        process_osm_relation();
        current_tags.clear();
        current_members.clear();
    }
}

void process_xml_node( xmlTextReaderPtr reader )
{
    xmlChar* name;
    name = xmlTextReaderName( reader );

    if ( name == NULL ) {
        name = xmlStrdup( ( const xmlChar* )"--" );
    }

    switch( xmlTextReaderNodeType( reader ) ) {
    case XML_READER_TYPE_ELEMENT:
        open_element( reader, name );

        if ( xmlTextReaderIsEmptyElement( reader ) ) {
            close_element( name );
        }

        break;
    case XML_READER_TYPE_END_ELEMENT:
        close_element( name );
        break;
    }

    xmlFree( name );
}

int streamFile( char* filename )
{
    xmlTextReaderPtr reader;
    int ret = 0;

    reader = xmlNewTextReaderFilename( filename );

    if ( reader != NULL ) {
        ret = xmlTextReaderRead( reader );

        while ( ret == 1 ) {
            process_xml_node( reader );
            ret = xmlTextReaderRead( reader );
        }

        if ( ret != 0 ) {
            die( "%s : failed to parse", filename );
        }

        xmlFreeTextReader( reader );
    }
    else {
        die( "Unable to open %s", filename );
    }

    return 0;
}

//bool node_equal(gconstpointer a, gconstpointer b)
//{
//    return memcmp(a, b, sizeof(double)<<1);
//}

void usage()
{
    printf( "Usage:\nosm2shp [-h] [-v] [-d <dest>] infile.osm\n" );
    printf( "-h for help.\n-v for verbose mode.\n-d to set destination directory (default: .)\n" );
    printf( "\nThis program converts OSM XML to ESRI Shape Files according to built-in rules.\n" );
    // test;
}

int main( int argc, char* argv[] )
{
    // never used int verbose=0;
    int i;
    int first_file_arg = argc;

    for ( i=1; i<argc; i++ ) {
        /*
        if ( strcmp("--verbose",argv[i]) == 0 || strcmp("-v",argv[i]) == 0 )
        {
            verbose=1;
        }

        else */ if ( strcmp( "--destination",argv[i] ) == 0 || strcmp( "-d",argv[i] ) == 0 ) {
            i++;

            if ( i==argc ) {
                usage();
                exit( EXIT_FAILURE );
            }

            outdir=argv[i];
        }
        else if ( argv[i][0] != '-' ) {
            first_file_arg = i;
            break;
        }
        else {
            usage();
            return ( strcmp( "--help",argv[i] ) == 0 || strcmp( "-h",argv[i] ) == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    for ( i=0; i<MAX_SHAPEFILES; i++ ) {
        shapefiles[i].shph = 0;
    }

    setup_shapefiles();

    for ( i=first_file_arg; i<argc; i++ ) {
        if ( streamFile( argv[i] ) ) {
            printf( "error while parsing '%s'\n",argv[i] );
            return EXIT_FAILURE;
        }
    }

    xmlCleanupParser();

    for ( i=0; i<MAX_SHAPEFILES; i++ ) {
        if ( shapefiles[i].shph ) {
            SHPClose( shapefiles[i].shph );
        }
        if ( shapefiles[i].dbfh ) {
            DBFClose( shapefiles[i].dbfh );
        }
    }

    return 0;

}
