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

/**
   Helper functions around libxml
 */

#ifndef TEMPUS_XML_HELPER_HH
#define TEMPUS_XML_HELPER_HH

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <libxml/xmlschemas.h>

#include <string>
#include <sstream>


///
/// Helper class designed to hold already-allocated pointers and call a deletion function
/// when the object is out of scope.
/// This is the way libxml works: it returns allocated pointers that have to be freed by the caller
///
/// There is no reference counting. Objets are "moved" from instances (as boost::auto_ptr does)
/// For example a = b transfers ownership from b to a and b is set to null
///
template <class T, void deletion_fct ( T* )>
class scoped_ptr {
public:
    scoped_ptr() : ptr_( 0 ) {}
    scoped_ptr( T* ptr ) : ptr_( ptr ) {}
    // copy constructor is a "move" constructor
    scoped_ptr( const scoped_ptr<T, deletion_fct>& p ) {
        ptr_ = p.ptr_;
        p.ptr_ = 0;
    }
    // "move" semantic
    scoped_ptr<T, deletion_fct>& operator = ( const scoped_ptr<T, deletion_fct>& p ) {
        if ( &p == this ) {
            return *this;
        }

        ptr_ = p.ptr_;
        p.ptr_ = 0;
        return *this;
    }

    virtual ~scoped_ptr() {
        deleteme();
    }
    T* get() {
        return ptr_;
    }
    void set( T* ptr ) {
        deleteme();
        ptr_ = ptr;
    }

protected:
    void deleteme() {
        if ( ptr_ ) {
            deletion_fct( ptr_ );
        }
    }
    mutable T* ptr_;
};

typedef scoped_ptr<xmlNode, xmlFreeNode> scoped_xmlNode;
typedef scoped_ptr<xmlDoc, xmlFreeDoc> scoped_xmlDoc;

///
/// XML helper namespace
/// @note the static member finction init() must be called once per thread
namespace XML {
class Schema {
public:
    Schema();
    Schema( const std::string& schema );
    virtual ~Schema();

    ///
    /// Load the schema
    void load( const std::string& );

    ///
    /// Throws a std::invalid_argument if the given node is not validated against the schema
    void ensure_validity( const xmlNode* node );

    ///
    /// Get the string representation of the schema
    /// @param[in] with_header Whether to include "<?xml>" header
    std::string to_string( bool with_header = false ) const;

private:
    xmlDoc*                           schema_doc_;
    xmlSchemaParserCtxt* parser_ctxt_;
    xmlSchema*                     schema_;
    xmlSchemaValidCtxt*   valid_ctxt_;
    // temp
    std::string schema_str_;
};

///
/// Returns a string that can be written as an XML text node
std::string escape_text( const std::string& message );

///
/// Outputs a node to a string, recursively
std::string to_string( const xmlNode* node, int indent_level = 0 );

///
/// Shortcut to xmlNewNode, using C++ std::string
xmlNode* new_node( const std::string& name );

///
/// Shortcut to xmlNewText, using C++ std::string
xmlNode* new_text( const std::string& text );

///
/// Shortcut to xmlNewProp, using C++ std::string
template <class T>
void new_prop( xmlNode* node, const std::string& key, T value )
{
    std::stringstream ss;
    ss << value;
    xmlNewProp( node, ( const xmlChar* )( key.c_str() ), ( const xmlChar* )( ss.str().c_str() ) );
}

///
/// Whether the node has a specified attribute
bool has_prop( const xmlNode* node, const std::string& key );

///
/// Shortcut to xmlGetProp, using C++ std::string
std::string get_prop( const xmlNode* node, const std::string& key );

///
/// Shortcut to xmlSetProp
void set_prop( xmlNode* node, const std::string& key, const std::string& value );

///
/// Shortcut to xmlAddChild
void add_child( xmlNode* node, xmlNode* child );

///
/// Get the next non text node
const xmlNode* get_next_nontext( const xmlNode* node );

///
/// To be called for each thread
int init();

///
/// Generic libxml error handling (private class). Accumulate errors in a string.
/// This is intended to be used to transform XML parsing errors to std::exceptions
struct ErrorHandling {
    static void accumulate_error( void* ctx, const char* msg, ... );
    static bool clear_errors_;
    static std::string xml_error_;
};
}

#endif
