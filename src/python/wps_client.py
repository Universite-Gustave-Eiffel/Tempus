#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
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
"""

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

# to_xml_indent : function that is called recursively to print out a python 'Tree' (pson)
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
        return [unicode(expr), True]

def to_xml( expr ):
    return to_xml_indent( expr, 0)[0]

#
# Convert a ElementTree XML tree to a Python expression (pson)
def to_pson( tree ):
    r = [ tree.tag ]
    if len(tree.attrib) > 0:
        r.append( tree.attrib )
    if len(tree) > 0:
        for child in tree:
            r.append( to_pson( child ) )
    else:
        if tree.text:
            r.append( tree.text )
    return r
        

# returns [ bool, exception_code ]
def get_wps_exception( str ):
    try:
        xml = ET.XML( str )
    except:
        return [False, '']

    try :
        exception = xml[0]
        code = exception.attrib['exceptionCode']
    except:
        return [False, '']
    return [True, code]

class HttpCgiConnection:
    def __init__( self, host, url ):
        self.conn = httplib.HTTPConnection( host )
        self.host = host
        self.url = url

    def reset(self):
        self.conn.close()
        self.conn.connect()
 
    def request( self, method, content ):
        self.reset()
        headers = {"Content-type" : "text/xml" }

        headers = {}
        url = self.url
        if method == "GET":
            url = self.url + "?" + content
        elif method == "POST":
            headers = {'Content-type' : 'text/xml' }
        else:
            raise RuntimeError("Unknown method " + method)

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
    # inputs : dictionary of argument_name -> xml_value
    #          if xml_value is a list, it is considered a 'complex' datum
    #          else, it is a 'literal' data
    def execute( self, identifier, inputs ):
        r = []
        for arg, v in inputs.items():
            if isinstance(v, list):
                data = [ 'ComplexData', {'mimeType' : 'text/xml', 'encoding' : 'UTF-8'}, v ]
            else:
                data = [ 'LiteralData', v ]

            r.append( [ 'Input',
                        [ 'Identifier', arg ],
                        [ 'Data', data ]] )

        body = [ 'wps:Execute', {'xmlns:wps':"http://www.opengis.net/wps/1.0.0", 'xmlns:ows':"http://www.opengis.net/ows/1.1", 'service':'WPS', 'version':'1.0.0'},
                 [ 'ows:Identifier', identifier ],
                 [ 'DataInputs' ] + r,
                 [ 'ResponseForm', 
                   [ 'RawDataOutput' ]]]

        x = to_xml( body )
#        print "Sent to WPS: ", x
        [status, msg] = self.conn.request( 'POST', x )
        if status == 502:
            raise RuntimeError( self, "The WPS server appears to be down ! " + msg)
        if status != 200:
            raise RuntimeError( self, msg )

#        print msg
        try:
            xml = ET.XML( msg )
        except ET.ParseError as e:
            raise RuntimeError( self, repr(e) + msg )

        outputs = xml[2] # ProcessOutputs
        outs = {}
        for output in outputs:
            identifier = output[0].text
            data = output[2][0][0]
            outs[identifier] = data
        return outs
