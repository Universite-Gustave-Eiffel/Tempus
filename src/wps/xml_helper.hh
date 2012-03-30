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
