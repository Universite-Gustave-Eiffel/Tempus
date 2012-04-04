#include <stdexcept>

#include "xml_helper.hh"

using namespace std;

int XML::init()
{
    xmlSetGenericErrorFunc( NULL, XML::accumulate_error );
}

// Force a call to XML::init() on startup
int XML::init_n_ = XML::init();

std::string XML::xml_error_;

bool XML::clear_errors_ = false;

void XML::accumulate_error( void* ctx, const char* msg, ...)
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

std::string XML::escape_text( const std::string& message )
{
    // escape the given error message
    xmlBuffer* buf = xmlBufferCreate();
    xmlNode * msg_node = xmlNewText( (const xmlChar*)message.c_str() );
    xmlNodeDump( buf, NULL, msg_node, 0, 0 );
    string escaped_msg = (const char*)xmlBufferContent( buf );
    xmlFreeNode( msg_node);
    xmlBufferFree( buf );
    return escaped_msg;
}

void XML::ensure_validity( xmlNode* node, const std::string& schema_str )
{
    std::string local_schema = "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
	+ schema_str +
	"</xs:schema>\n";
    scoped_xmlDoc schema_doc = xmlReadMemory(local_schema.c_str(), local_schema.size(), "schema.xml", NULL, 0);
    if (schema_doc.get() == NULL) {
	clear_errors_ = true;
	throw std::invalid_argument( xml_error_ );
    }
    scoped_ptr<xmlSchemaParserCtxt, xmlSchemaFreeParserCtxt> parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc.get());
    if (parser_ctxt.get() == NULL) {
	// unable to create a parser context for the schema
	clear_errors_ = true;
	throw std::invalid_argument( xml_error_ );
    }
    scoped_ptr<xmlSchema, xmlSchemaFree> schema = xmlSchemaParse(parser_ctxt.get());
    if (schema.get() == NULL) {
	// the schema itself is not valid
	clear_errors_ = true;
	throw std::invalid_argument( xml_error_ );
    }
    scoped_ptr<xmlSchemaValidCtxt, xmlSchemaFreeValidCtxt> valid_ctxt = xmlSchemaNewValidCtxt(schema.get());
    if (valid_ctxt.get() == NULL) {
	// unable to create a validation context for the schema
	clear_errors_ = true;
	throw std::invalid_argument( xml_error_ );
    }
    
    // create a new Doc from the subtree node
    scoped_xmlDoc subtree_doc = xmlNewDoc((const xmlChar*)"1.0");
    xmlDocSetRootElement( subtree_doc.get(), node );
    
    int is_valid = (xmlSchemaValidateDoc( valid_ctxt.get(), subtree_doc.get() ) == 0);
    if ( !is_valid )
    {
	clear_errors_ = true;
	throw std::invalid_argument( xml_error_ );
    }
    // remove the root node from the subtree document to prevent double free()
    subtree_doc.get()->children = NULL;
}

xmlNode* XML::get_next_nontext( xmlNode* node )
{
    while ( node && xmlNodeIsText(node) )
	node = node->next;
    return node;
}
