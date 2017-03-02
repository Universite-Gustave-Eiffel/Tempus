/*
Copyright (c) 2012, Canal TP
Copyright (C) 2016 Oslandia <infos@oslandia.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Canal TP nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CANAL TP BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <stdint.h>

#ifdef _MSC_VER
#include <Winsock2.h> // ntohl
#else
#include <netinet/in.h> // for ntohl
#endif

#include <zlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <ios>

// this describes the low-level blob storage
#include "fileformat.pb.h"
// this describes the high-level OSM objects
#include "osmformat.pb.h"
// the maximum size of a blob header in bytes
const int max_blob_header_size = 64 * 1024; // 64 kB
// the maximum size of an uncompressed blob in bytes
const int max_uncompressed_blob_size = 32 * 1024 * 1024; // 32 MB
// resolution for longitude/latitude used for conversion
// between representation as double and as int
const int lonlat_resolution = 1000 * 1000 * 1000;

namespace CanalTP {

// Represents the key/values of an object
typedef std::map<std::string, std::string> Tags;

// References of a relation
struct Reference {
    OSMPBF::Relation::MemberType member_type; // type de la relation
    uint64_t member_id; // OSMID
    std::string role; // le role

    Reference() {}
    Reference(OSMPBF::Relation::MemberType member_type, uint64_t member_id, std::string role) :
        member_type(member_type), member_id(member_id), role(role)
    {}
};

typedef std::vector<Reference> References;

struct warn {
    warn() {std::cout << "\033[33m[WARN] ";}
    template<typename T>warn & operator<<(const T & t){ std::cout << t; return *this;}
    ~warn() {std::cout << "\033[0m" << std::endl;}
};

struct info {
    info() {std::cout << "\033[32m[INFO] ";}
    template<typename T>info & operator<<(const T & t){ std::cout << t; return *this;}
    ~info() {std::cout << "\033[0m" << std::endl;}
};

struct fatal {
    fatal() {std::cout << "\033[31m[FATAL] ";}
    template<typename T>fatal & operator<<(const T & t){ std::cout << t; return *this;}
    ~fatal() {std::cout << "\033[0m" << std::endl; exit(1);}
};

template<typename T>
Tags get_tags(T object, const OSMPBF::PrimitiveBlock &primblock){
    Tags result;
    for(int i = 0; i < object.keys_size(); ++i){
        uint64_t key = object.keys(i);
        uint64_t val = object.vals(i);
        std::string key_string = primblock.stringtable().s(key);
        std::string val_string = primblock.stringtable().s(val);
        result[key_string] = val_string;
    }
    return result;
}

template <typename Progressor>
struct Parser
{
    enum BlockType
    {
        NodeBlock = 1,
        WayBlock = 2,
        RelationBlock = 4
    };

    ///
    /// Assuming the file is sorted (nodes then ways then relations)
    /// return the offset of the nodes/ways and ways/relations limit
    void get_file_offsets( off_t& ways_offset, off_t& relations_offset ){
        Progressor progressor;
        BlockType current_block = NodeBlock;
        while(!this->file.eof() && !finished) {
            off_t tellp = file.tellg();
            progressor( tellp, fsize );
            OSMPBF::BlobHeader header = this->read_header();
            if(!this->finished){
                int32_t sz = this->read_blob(header);
                if(header.type() == "OSMData") {
                    int b = enum_primitiveblock( sz );
                    if ( (current_block == NodeBlock) && (b & WayBlock) ) {
                        ways_offset = tellp;
                        current_block = WayBlock;
                    }
                    else if ( (current_block == WayBlock) && (b & RelationBlock) ) {
                        relations_offset = tellp;
                        current_block = RelationBlock;
                    }
                }
                else if(header.type() == "OSMHeader"){
                }
                else {
                    warn() << "  unknown blob type: " << header.type();
                }
            }
        }
    }

    ///
    /// Assuming the file is sorted (nodes then ways then relations)
    /// return the offset of the nodes/ways and ways/relations limit
    /// This method uses a binary search and is faster than get_file_offsets
    void get_file_offsets2( off_t& ways_offset, off_t& relations_offset ){
        ways_offset = binary_search_pattern_( 0, fsize, WayBlock );
        relations_offset = binary_search_pattern_( 0, fsize, RelationBlock );
    }

    void stats( size_t& n_nodes, size_t& n_ways, size_t& n_highways, size_t& n_relations )
    {
        off_t fsize;
        this->file.seekg(0, std::ios_base::end);
        fsize = this->file.tellg();
        this->file.seekg( 0, std::ios_base::beg );

        Progressor progressor;
        while(!this->file.eof() && !this->finished) {
            progressor( this->file.tellg(), fsize );
            OSMPBF::BlobHeader header = this->read_header();
            if(!this->finished){
                int32_t sz = this->read_blob(header);
                if(header.type() == "OSMData") {
                    this->stat_primitiveblock(sz, n_nodes, n_ways, n_highways, n_relations);
                }
                else if(header.type() == "OSMHeader"){
                }
                else {
                    warn() << "  unknown blob type: " << header.type();
                }
            }
        }
    }

    Parser(const std::string & filename )
        : file(filename.c_str(), std::ios::binary ), finished(false)
    {
        if(!file.is_open())
            fatal() << "Unable to open the file " << filename;
        buffer = new char[max_uncompressed_blob_size];
        unpack_buffer = new char[max_uncompressed_blob_size];

        // get file size
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        fsize = in.tellg();
    }

    virtual ~Parser(){
        delete[] buffer;
        delete[] unpack_buffer;
        // google::protobuf::ShutdownProtobufLibrary();
    }
protected:
    std::ifstream file;
    char* buffer;
    char* unpack_buffer;
    bool finished;
    off_t fsize;

    OSMPBF::BlobHeader read_header(){
        int32_t sz;
        OSMPBF::BlobHeader result;

        // read the first 4 bytes of the file, this is the size of the blob-header
        if( !file.read((char*)&sz, 4) ){
            this->finished = true;
            return result;
        }

        sz = ntohl(sz);// convert the size from network byte-order to host byte-order

        if(sz > max_blob_header_size)
            fatal() << "blob-header-size is bigger then allowed " << sz << " > " << max_blob_header_size;

        this->file.read(this->buffer, sz);
        if(!this->file.good())
            fatal() << "unable to read blob-header from file";

        // parse the blob-header from the read-buffer
        if(!result.ParseFromArray(this->buffer, sz))
            fatal() << "unable to parse blob header";
        return result;
    }

    int32_t read_blob(const OSMPBF::BlobHeader & header){
        OSMPBF::Blob blob;
        // size of the following blob
        int32_t sz = header.datasize();

        if(sz > max_uncompressed_blob_size)
            fatal() << "blob-size is bigger then allowed";

        if(!this->file.read(buffer, sz))
            fatal() << "unable to read blob from file";
        if(!blob.ParseFromArray(this->buffer, sz))
            fatal() << "unable to parse blob";

        // if the blob has uncompressed data
        if(blob.has_raw()) {
            // size of the blob-data
            sz = blob.raw().size();

            // check that raw_size is set correctly
            if(sz != blob.raw_size())
                warn() << "  reports wrong raw_size: " << blob.raw_size() << " bytes";

            memcpy(unpack_buffer, buffer, sz);
            return sz;
        }


        if(blob.has_zlib_data()) {
            sz = blob.zlib_data().size();

            z_stream z;
            z.next_in   = (unsigned char*) blob.zlib_data().c_str();
            z.avail_in  = sz;
            z.next_out  = (unsigned char*) unpack_buffer;
            z.avail_out = blob.raw_size();
            z.zalloc    = Z_NULL;
            z.zfree     = Z_NULL;
            z.opaque    = Z_NULL;

            if(inflateInit(&z) != Z_OK) {
                fatal() << "failed to init zlib stream";
            }
            if(inflate(&z, Z_FINISH) != Z_STREAM_END) {
                fatal() << "failed to inflate zlib stream";
            }
            if(inflateEnd(&z) != Z_OK) {
                fatal() << "failed to deinit zlib stream";
            }
            return z.total_out;
        }

        if(blob.has_lzma_data()) {
            fatal() << "lzma-decompression is not supported";
        }
        return 0;
    }

    int enum_primitiveblock(int32_t sz) {
        OSMPBF::PrimitiveBlock primblock;
        if(!primblock.ParseFromArray(this->unpack_buffer, sz))
            fatal() << "unable to parse primitive block";

        int block_types = 0;
        for(int i = 0, l = primblock.primitivegroup_size(); i < l; i++) {
            OSMPBF::PrimitiveGroup pg = primblock.primitivegroup(i);
            if ( pg.relations_size() ) {
                block_types |= RelationBlock;
            }
            if ( pg.ways_size() ) {
                block_types |= WayBlock;
            }
            if ( pg.nodes_size() || pg.has_dense() ) {
                block_types |= NodeBlock;
            }
        }
        return block_types;
    }

    off_t next_block()
    {
        const char pattern[] = "\0\0\0\x0d\x0a\x07OSMData";
        const size_t bsize = 13;
        char buf[bsize+1] = {0};
        char c;
        while (file.get(c)) {
            memmove( &buf[0], &buf[1], bsize - 1 );
            buf[bsize-1] = c;
            if ( memcmp( buf, pattern, bsize ) == 0 ) {
                file.seekg( static_cast<off_t>( file.tellg() ) - static_cast<off_t>( bsize ), file.beg );
                return file.tellg();
            }
        }
        return -1;
    }

    off_t linear_search_pattern_( off_t start, off_t end, BlockType block_type )
    {
        file.seekg( start );
        OSMPBF::BlobHeader header;
        off_t nxt = next_block();
        if ( nxt > end )
            return -1;

        off_t found_off = 0;
        int b = 0;
        while ( b < block_type ) {
            found_off = file.tellg();
            if ( found_off > end )
                return -1;
            header = this->read_header();
            if(!this->finished){
                int32_t sz = this->read_blob(header);
                if(header.type() == "OSMData") {
                    b = enum_primitiveblock( sz );
                }
            }
        }
        return found_off;
    }

    off_t binary_search_pattern_( off_t start, off_t end, BlockType block_type )
    {
        int iblock_type = static_cast<int>( block_type );
        off_t half = (start + end) / 2;
        if ( file.eof() )
            file.clear();
        file.seekg( half );
        off_t nxt = next_block();
        if ( nxt == -1 )
            return -1;
        if ( nxt > end ) {
            // the next block is past the end,
            // this means we need a linear search between start and end
            return linear_search_pattern_( start, end, block_type );
        }

        // read two headers so that
        // header 1 is not of type block_type
        // and header 2 is of type block_type
        int b1 = 0, b2 = 0;
        OSMPBF::BlobHeader header = this->read_header();
        if(!this->finished){
            int32_t sz = this->read_blob(header);
            if(header.type() == "OSMData") {
                b1 = enum_primitiveblock( sz );
            }
        }
        off_t found_off = file.tellg();
        header = this->read_header();
        if(!this->finished){
            int32_t sz = this->read_blob(header);
            if(header.type() == "OSMData") {
                b2 = enum_primitiveblock( sz );
            }
        }
        this->finished = false;
        if ( ((b1 & iblock_type) == 0) && (b2 & iblock_type) ) {
            return found_off;
        }
        else if ( b1 >= block_type ) {
            return binary_search_pattern_( start, half, block_type );
        }
        else if ( b2 < block_type ) {
            return binary_search_pattern_( half, end, block_type );
        }
        return -1;
    }

    void stat_primitiveblock(int32_t sz, size_t& n_nodes, size_t& n_ways, size_t& n_highways, size_t& n_relations) {
        OSMPBF::PrimitiveBlock primblock;
        if(!primblock.ParseFromArray(this->unpack_buffer, sz))
            fatal() << "unable to parse primitive block";

        for(int i = 0, l = primblock.primitivegroup_size(); i < l; i++) {
            OSMPBF::PrimitiveGroup pg = primblock.primitivegroup(i);

            n_nodes += pg.nodes_size();

            // Dense Nodes
            if(pg.has_dense()) {
                OSMPBF::DenseNodes dn = pg.dense();
                n_nodes += dn.id_size();
            }

            n_ways += pg.ways_size();
            for(int i = 0; i < pg.ways_size(); ++i) {
                OSMPBF::Way w = pg.ways(i);

                for(int j = 0; j < w.keys_size(); ++j){
                    uint64_t key = w.keys(j);
                    uint64_t val = w.vals(j);
                    std::string key_string = primblock.stringtable().s(key);
                    std::string val_string = primblock.stringtable().s(val);
                    if ( key_string == "highway" )
                        n_highways++;
                }
            }

            n_relations += pg.relations_size();
        }
    }
};

template<typename Visitor, typename Progressor>
struct ParserWithVisitor : public Parser<Progressor> {

    ParserWithVisitor(const std::string & filename, Visitor & visitor )
        : Parser<Progressor>( filename ), visitor( visitor )
    {
    }

    void parse( off_t start_offset = 0, off_t end_offset = 0 )
    {
        off_t fsize;
        if ( end_offset == 0 ) {
            this->file.seekg(0, std::ios_base::end);
            end_offset = this->file.tellg();
        }
        fsize = end_offset - start_offset;
        this->file.seekg( start_offset, std::ios_base::beg );

        Progressor progressor;
        while(!this->file.eof() && !this->finished && ( !end_offset || this->file.tellg() <= end_offset ) ) {
            progressor( static_cast<off_t>( this->file.tellg() ) - start_offset, fsize );
            //std::cout << std::hex << this->file.tellg() << std::endl;
            OSMPBF::BlobHeader header = this->read_header();
            if(!this->finished){
                int32_t sz = this->read_blob(header);
                if(header.type() == "OSMData") {
                    this->parse_primitiveblock(sz);
                }
                else if(header.type() == "OSMHeader"){
                }
                else {
                    warn() << "  unknown blob type: " << header.type();
                }
            }
        }
    }

private:
    Visitor & visitor;

    void parse_primitiveblock(int32_t sz) {
        OSMPBF::PrimitiveBlock primblock;
        if(!primblock.ParseFromArray(this->unpack_buffer, sz))
            fatal() << "unable to parse primitive block";

        for(int i = 0, l = primblock.primitivegroup_size(); i < l; i++) {
            OSMPBF::PrimitiveGroup pg = primblock.primitivegroup(i);

            // Simple Nodes
            for(int i = 0; i < pg.nodes_size(); ++i) {
                OSMPBF::Node n = pg.nodes(i);
                double lon = 0.000000001 * (primblock.lon_offset() + (primblock.granularity() * n.lon())) ;
                double lat = 0.000000001 * (primblock.lat_offset() + (primblock.granularity() * n.lat())) ;
                visitor.node_callback(n.id(), lon, lat, get_tags(n, primblock));
            }

            // Dense Nodes
            if(pg.has_dense()) {
                OSMPBF::DenseNodes dn = pg.dense();
                uint64_t id = 0;
                double lon = 0;
                double lat = 0;

                int current_kv = 0;

                for(int i = 0; i < dn.id_size(); ++i) {
                    id += dn.id(i);
                    lon +=  0.000000001 * (primblock.lon_offset() + (primblock.granularity() * dn.lon(i)));
                    lat +=  0.000000001 * (primblock.lat_offset() + (primblock.granularity() * dn.lat(i)));

                    Tags tags;
                    while (current_kv < dn.keys_vals_size() && dn.keys_vals(current_kv) != 0){
                        uint64_t key = dn.keys_vals(current_kv);
                        uint64_t val = dn.keys_vals(current_kv + 1);
                        std::string key_string = primblock.stringtable().s(key);
                        std::string val_string = primblock.stringtable().s(val);
                        current_kv += 2;
                        tags[key_string] = val_string;
                    }
                    ++current_kv;
                    visitor.node_callback(id, lon, lat, tags);
                }
            }

            for(int i = 0; i < pg.ways_size(); ++i) {
                OSMPBF::Way w = pg.ways(i);

                uint64_t ref = 0;
                std::vector<uint64_t> refs;
                for(int j = 0; j < w.refs_size(); ++j){
                    ref += w.refs(j);
                    refs.push_back(ref);
                }
                uint64_t id = w.id();
                visitor.way_callback(id, get_tags(w, primblock), refs);
            }


            for(int i=0; i < pg.relations_size(); ++i){
                OSMPBF::Relation rel = pg.relations(i);
                uint64_t id = 0;
                References refs;

                for(int l = 0; l < rel.memids_size(); ++l){
                    id += rel.memids(l);
                    refs.push_back(Reference(rel.types(l), id, primblock.stringtable().s(rel.roles_sid(l))));
                }

                visitor.relation_callback(rel.id(), get_tags(rel, primblock), refs);
            }
        }
    }

};

struct NullProgressor
{
    void operator()( size_t, size_t )
    {}
    void end()
    {}
};

template<typename Progressor = NullProgressor>
void osm_pbf_offsets(const std::string & filename, off_t& ways_offset, off_t& relations_offset )
{
    Parser<Progressor> p( filename );
    p.get_file_offsets2( ways_offset, relations_offset );
}

template<typename Visitor, typename Progressor = NullProgressor>
void read_osm_pbf( const std::string & filename, Visitor & visitor, off_t start_offset = 0, off_t end_offset = 0 )
{
    ParserWithVisitor<Visitor, Progressor> p(filename, visitor);
    p.parse( start_offset, end_offset );
}

template<typename Progressor = NullProgressor>
void stats_osm_pbf( const std::string & filename, size_t& n_nodes, size_t& n_ways, size_t& n_highways, size_t& n_relations )
{
    n_nodes = 0;
    n_ways = 0;
    n_highways = 0;
    n_relations = 0;
    Parser<Progressor> p(filename);
    p.stats( n_nodes, n_ways, n_highways, n_relations );
}

}
