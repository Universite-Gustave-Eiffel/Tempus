// XML Helper
// (c) 2012 Oslandia - Hugo Mercier <hugo.mercier@oslandia.com>
// MIT Licence
/**
   Helper functions around libxml
 */

#ifndef TEMPUS_XML_HELPER_HH
#define TEMPUS_XML_HELPER_HH

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <string>

///
/// Helper class designed to hold already-allocated pointers and call a deletion function
/// when the object is out of scope.
/// This is the way libxml works: it returns allocated pointers that have to be freed by the caller
///
/// There is no reference counting. Objets are "moved" from instances (as boost::auto_ptr does)
/// For example a = b transfers ownership from b to a and b is set to null

template <class T, void deletion_fct (T*)>
class scoped_ptr
{
public:
    scoped_ptr() : ptr_(0) {}
    scoped_ptr( T* ptr ) : ptr_(ptr) {}
    // copy constructor is a "move" constructor
    scoped_ptr( const scoped_ptr<T, deletion_fct>& p )
    {
	ptr_ = p.ptr_;
	p.ptr_ = 0;
    }
    // "move" semantic
    scoped_ptr<T, deletion_fct>& operator = ( const scoped_ptr<T, deletion_fct>& p )
    {
	if ( &p == this )
	    return *this;

	ptr_ = p.ptr_;
	p.ptr_ = 0;
	return *this;
    }
    
    virtual ~scoped_ptr()
    {
	deleteme();
    }
    T* get() { return ptr_; }
    void set( T* ptr ) { deleteme(); ptr_ = ptr; }
    
protected:
    void deleteme()
    {
	if ( ptr_ )
	    deletion_fct( ptr_ );
    }
    mutable T* ptr_;
};

typedef scoped_ptr<xmlNode, xmlFreeNode> scoped_xmlNode;
typedef scoped_ptr<xmlDoc, xmlFreeDoc> scoped_xmlDoc;

class XML
{
public:
    ///
    /// Returns a string that can be written as an XML text node
    static std::string escape_text( const std::string& message );

    ///
    /// Throws a std::invalid_argument if the given node is not validated against the schema
    static void ensure_validity( xmlNode* node, const std::string& schema_str );

    ///
    /// Get the next non text node
    static xmlNode* get_next_nontext( xmlNode* node );

protected:
    ///
    /// Generic libxml error handling. Accumulate errors in a string.
    /// This is intended to be used to transform XML parsing errors to std::exceptions
    static void accumulate_error( void* ctx, const char* msg, ...);

    static bool clear_errors_;
    static std::string xml_error_;

    static int init_n_;
    static int init();
};

#endif
