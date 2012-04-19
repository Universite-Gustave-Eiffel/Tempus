"""
WPS client classes.
Shared by the command line tests and by the QGis plugin
"""
import httplib
import urllib
from xml.etree import ElementTree as ET

# Compact representation of XML by means of Python expressions
#
# A Node is either
# * a string representing a text node
# * or a list representing a node with :
#   * its name as first arg
#   * a dictionary of attributes as optional second arg
#   * children Nodes as following arguments (possibly empty)
#
# Example :
# [ 'a' { 'href' : "http://www.example.org" } [ 'ul' [ 'li', "item0" ], ['li', 'item1'] ] ]
# is equivalent to :
# <a href="http://www.example.org>
#   <ul>
#     <li>item0</li>
#     <li>item1</li>
#   </ul>
# </a>

# to_xml_indent : function that is called recursively to print out a python 'Tree'
# indent_size : recursive argument
# returns a [ string, boolean ] where the boolean indicates whether the string is a text node or not
def to_xml_indent( expr, indent_size ):
    if isinstance( expr, list ):
        name = expr[0]
        
        # attributes
        if len(expr) == 1:
            attrs = {}
            children = []
        elif isinstance( expr[1], dict ):
            attrs = expr[1]
            children = expr[2:]
        else:
            attrs = {}
            children = expr[1:]

        o_str = name
        for k,v in attrs.items():
            # FIXME : must escape here
            o_str += " %s=\"%s\"" % (k,v)

        if children == []:
            return [' ' * indent_size + '<' + o_str + "/>\n", False]

        children_str = ''
        is_text = False
        if len(children) == 1:
            [children_str, is_text] = to_xml_indent( children[0], indent_size + 2)
        else:
            for child in children:
                [child_str, ignored] = to_xml_indent( child, indent_size + 2 )
                children_str += child_str
        if is_text:
            return [' ' * indent_size + "<" + o_str +">" + children_str + "</" + name + ">\n", False]
        return [' ' * indent_size + "<" + o_str +">\n" + children_str + ' ' * indent_size + "</" + name + ">\n", False]
    else:
        # FIXME : must escape
        return [str(expr), True]

def to_xml( expr ):
    return to_xml_indent( expr, 0)[0]

class HttpCgiConnection:
    def __init__( self, host, url ):
        self.conn = httplib.HTTPConnection( host )
        self.host = host
        self.url = url

    def request( self, method, content ):
        headers = {"Content-type" : "text/xml" }

        headers = {}
        url = self.url
        if method == "GET":
            url = self.url + "?" + content
        elif method == "POST":
            headers = {'Content-type' : 'text/xml' }
        else:
            raise "Unknown method " + method

        self.conn.request( method, url, content, headers )
        r1 = self.conn.getresponse()
        return [r1.status, r1.read()]
        
class WPSClient:

    def __init__ ( self, connection ):
        self.conn = connection

    def get_capabilities( self ):
        return self.conn.request( 'GET', 'service=wps&version=1.0.0&request=GetCapabilities' )

    def describe_process( self, identifier ):
        return self.conn.request( 'GET', 'service=wps&version=1.0.0&request=DescribeProcess&identifier=' + identifier )

    #
    # inputs : dictionary of argument_name -> [ is_complex ?, xml value ]
    def execute( self, identifier, inputs ):
        r = []
        for arg, v in inputs.items():
            is_complex = v[0]
            xml_value = v[1]
            if is_complex:
                data = [ 'ComplexData', {'mimeType' : 'text/xml', 'encoding' : 'UTF-8'}, xml_value ]
            else:
                data = [ 'LiteralData', xml_value ]

            r.append( [ 'Input',
                        [ 'Identifier', arg ],
                        [ 'Data', data ]] )

        body = [ 'wps:Execute', {'xmlns:wps':"http://www.opengis.net/wps/1.0.0", 'xmlns:ows':"http://www.opengis.net/ows/1.1", 'service':'WPS', 'version':'1.0.0'},
                 [ 'ows:Identifier', identifier ],
                 [ 'DataInputs' ] + r,
                 [ 'ResponseForm', 
                   [ 'RawDataOutput' ]]]

        x = to_xml( body )
        #print "Sent to WPS: ", x
        [status, msg] = self.conn.request( 'POST', x )
        if status != 200:
            raise RuntimeError( self, "During execution of '" + identifier + "': " + msg )

        xml = ET.XML( msg )
        outputs = xml[2] # ProcessOutputs
        outs = {}
        for output in outputs:
            identifier = output[0].text
            data = output[2][0][0]
            outs[identifier] = data
        print outs
        return outs
