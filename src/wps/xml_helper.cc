#include <stdexcept>

#include "xml_helper.hh"

using namespace std;

namespace XML
{

int init()
{
    xmlSetGenericErrorFunc( NULL, ErrorHandling::accumulate_error );
    return 0;
}

std::string ErrorHandling::xml_error_;
bool ErrorHandling::clear_errors_ = false;
void ErrorHandling::accumulate_error( void* /*ctx*/, const char* msg, ...)
{
    if ( clear_errors_ )
    {
	xml_error_.clear();
	clear_errors_ = false;
    }
#define TMP_BUF_SIZE 1024
    char buffer[TMP_BUF_SIZE];
    va_list arg_ptr;
    
    va_start(arg_ptr, msg);
    vsnprintf(buffer, TMP_BUF_SIZE, msg, arg_ptr);
    va_end(arg_ptr);
    
    xml_error_ += buffer;
#undef TMP_BUF_SIZE
}

std::string escape_text( const std::string& message )
{
    // escape the given error message
    scoped_ptr<xmlBuffer, xmlBufferFree> buf = xmlBufferCreate();
    scoped_xmlNode msg_node = xmlNewText( (const xmlChar*)message.c_str() );
    xmlNodeDump( buf.get(), NULL, msg_node.get(), 0, 0 );
    return (const char*)xmlBufferContent( buf.get() );
}

std::string to_string( xmlNode* node, int indent_level )
{
    scoped_ptr<xmlBuffer, xmlBufferFree> buf = xmlBufferCreate();
    xmlNodeDump( buf.get(), NULL, node, indent_level, 1 );
    // avoid a string copy, thanks to scoped_ptr
    return (const char*)xmlBufferContent( buf.get() );
}

std::string to_string( xmlDoc* doc )
{
    xmlChar *buf;
    int size;
    xmlDocDumpFormatMemory( doc, &buf, &size, 0 );
    std::string r = (const char*)buf;
    xmlFree( buf );
    return r;
}

xmlNode* new_node( const std::string& name )
{
    return xmlNewNode( NULL, (const xmlChar*)name.c_str() );
}

xmlNode* new_text( const std::string& text )
{
    return xmlNewText( (const xmlChar*)text.c_str() );
}

std::string get_prop( const xmlNode* node, const std::string& key )
{
    return (const char*)xmlGetProp( const_cast<xmlNode*>(node), (const xmlChar*)(key.c_str()) );
}

void add_child( xmlNode* node, xmlNode* child )
{
    xmlAddChild( node, child );
}

Schema::Schema() :
    schema_doc_(0),
    parser_ctxt_(0),
    schema_(0),
    valid_ctxt_(0),
    schema_str_("")
{
}

Schema::Schema( const std::string& schema_str )
{
    load( schema_str );
}

Schema::~Schema()
{
    if ( valid_ctxt_ ) {
        xmlSchemaFreeValidCtxt( valid_ctxt_ );
    }
    if ( schema_ ) {
        xmlSchemaFree( schema_ );
    }
    if ( parser_ctxt_ ) {
        xmlSchemaFreeParserCtxt( parser_ctxt_ );
    }
    if ( schema_doc_ ) {
        xmlFreeDoc( schema_doc_ );
    }
}

void Schema::load( const std::string& schema )
{ 
    schema_str_ = schema;
    std::string local_schema = "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
	+ schema_str_ +
	"</xs:schema>\n";
    schema_doc_ = xmlReadMemory(local_schema.c_str(), local_schema.size(), "schema.xml", NULL, 0);
    if (schema_doc_ == NULL) {
	ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
#if 0
    schema_doc_ = xmlReadFile( filename.c_str(), /* encoding ?*/ NULL, /* options */ 0 );
    if (schema_doc_ == NULL) {
	ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
#endif
    parser_ctxt_ = xmlSchemaNewDocParserCtxt(schema_doc_);
    if (parser_ctxt_ == NULL) {
	// unable to create a parser context for the schema
	ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
    schema_ = xmlSchemaParse(parser_ctxt_);
    if (schema_ == NULL) {
	// the schema itself is not valid
	ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
    valid_ctxt_ = xmlSchemaNewValidCtxt(schema_);
    if (valid_ctxt_ == NULL) {
	// unable to create a validation context for the schema
	ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
}

std::string Schema::to_string( bool with_header ) const
{
    if ( with_header ) {
        return "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
            + schema_str_ +
            "</xs:schema>\n";
    }
    return schema_str_;
}

void Schema::ensure_validity( const xmlNode* node )
{
    if ( valid_ctxt_ == 0 ) {
        return;
    }
    // create a new Doc from the subtree node
    scoped_xmlDoc subtree_doc = xmlNewDoc((const xmlChar*)"1.0");
    // ! xmlDocSetRootElement update every nodes, use simple pointer affectation instead
    subtree_doc.get()->children = const_cast<xmlNode*>(node);
    int is_valid = (xmlSchemaValidateDoc( valid_ctxt_, subtree_doc.get() ) == 0);
    // remove the root node from the subtree document to prevent double free()
    subtree_doc.get()->children = NULL;
    if ( !is_valid )
    {
        ErrorHandling::clear_errors_ = true;
	throw std::invalid_argument( ErrorHandling::xml_error_ );
    }
}

const xmlNode* get_next_nontext( const xmlNode* node )
{
    while ( node && xmlNodeIsText(const_cast<xmlNode*>(node)) )
	node = node->next;
    return node;
}

} // XML namespace
