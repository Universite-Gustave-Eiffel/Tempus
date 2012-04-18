# -*- coding: utf-8 -*-
"""
/***************************************************************************
 IfsttarRouting
                                 A QGIS plugin
 Get routing informations with various algorithms
                              -------------------
        begin                : 2012-04-12
        copyright            : (C) 2012 by Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""
# Import the PyQt and QGIS libraries
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
# Initialize Qt resources from file resources.py
import resources_rc
# Import the code for the dialog
from ifsttarroutingdock import IfsttarRoutingDock

import re
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

#        if r == []:
#            r = ""
        print r
        body = [ 'wps:Execute', {'xmlns:wps':"http://www.opengis.net/wps/1.0.0", 'xmlns:ows':"http://www.opengis.net/ows/1.1", 'service':'WPS', 'version':'1.0.0'},
                 [ 'ows:Identifier', identifier ],
                 [ 'DataInputs' ] + r,
                 [ 'ResponseForm', 
                   [ 'RawDataOutput' ]]]

        x = to_xml( body )
        print "Sent to WPS: ", x
        return self.conn.request( 'POST', x )

    # argument : [status, msg]
    def check_execute_response( self, arg ):
        [status, msg] = arg
        if status != 200:
            raise RuntimeError(self, "Execute response: " + msg)
        print "Received: " + msg

def connectProcessingButton( btn, fct ):
    QObject.connect( btn, SIGNAL("clicked()"), lambda : onProcessingButton(btn, fct) )

# Generic processing button: first disable the button, do the processing, then enable back the button
def onProcessingButton( btn, fct ):
    btn.setEnabled(False)
    fct()
    btn.setEnabled(True)

class IfsttarRouting:

    def __init__(self, iface):
        # Save reference to the QGIS interface
        self.iface = iface
        # Create the dialog and keep reference
        self.dlg = IfsttarRoutingDock()
        self.canvas = self.iface.mapCanvas()
        self.clickTool = QgsMapToolEmitPoint(self.canvas)
        # initialize plugin directory
        self.plugin_dir = QFileInfo(QgsApplication.qgisUserDbFilePath()).path() + "/python/plugins/ifsttarrouting"
        # initialize locale
        localePath = ""
        locale = QSettings().value("locale/userLocale").toString()[0:2]
       
        if QFileInfo(self.plugin_dir).exists():
            localePath = self.plugin_dir + "/i18n/ifsttarrouting_" + locale + ".qm"

        if QFileInfo(localePath).exists():
            self.translator = QTranslator()
            self.translator.load(localePath)

            if qVersion() > '4.3.3':
                QCoreApplication.installTranslator(self.translator)
   

    def initGui(self):
        # Create action that will start plugin configuration
        self.action = QAction(QIcon(":/plugins/ifsttarrouting/icon.png"), \
            u"Compute route", self.iface.mainWindow())
        # connect the action to the run method
        QObject.connect(self.action, SIGNAL("triggered()"), self.run)

        QObject.connect(self.dlg.ui.connectBtn, SIGNAL("clicked()"), self.onConnect)
        QObject.connect(self.dlg.ui.originSelectBtn, SIGNAL("clicked()"), self.onOriginSelect)
        QObject.connect(self.dlg.ui.destinationSelectBtn, SIGNAL("clicked()"), self.onDestinationSelect)
        QObject.connect(self.dlg.ui.computeBtn, SIGNAL("clicked()"), self.onCompute)
        QObject.connect(self.dlg.ui.verticalTabWidget, SIGNAL("currentChanged( int )"), self.onTabChanged)
        connectProcessingButton( self.dlg.ui.buildBtn, self.onBuildGraphs )

        self.originPoint = QgsPoint()
        self.destinationPoint = QgsPoint()

        # Add toolbar button and menu item
        self.iface.addToolBarIcon(self.action)
        self.iface.addPluginToMenu(u"&Compute route", self.action)
        self.iface.addDockWidget( Qt.LeftDockWidgetArea, self.dlg )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 1, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 2, False )

    # when the 'connect' button get clicked
    def onConnect( self ):
        # get the wps url
        g = re.search( 'http://([^/]+)(.*)', str(self.dlg.ui.wpsUrlText.text()) )
        host = g.group(1)
        path = g.group(2)
        connection = HttpCgiConnection( host, path )
        self.wps = WPSClient(connection)

        [status, msg] = self.wps.execute( 'state', {} )
        self.wps.check_execute_response( [status, msg] )
        xml = ET.XML( msg )
        outputs = xml[2]
        self.state = 0
        for output in outputs:
            content = output[2][0][0]            
            if content.tag == 'db_options':
                if content.text is not None:
                    self.dlg.ui.dbOptionsText.setText( content.text )
            elif content.tag == 'state':
                self.state = int( content.text )
                self.dlg.ui.stateText.setText( content.text )

        # get plugin list
        [status, msg] = self.wps.execute( 'plugin_list', {} )
        self.wps.check_execute_response( [status, msg] )
        print msg
        xml = ET.XML( msg )
        plugins = xml[2][0][2][0][0]
        self.dlg.ui.pluginCombo.clear()
        for plugin in plugins:
            self.dlg.ui.pluginCombo.insertItem(0, plugin.attrib['name'] )
            
        # enable db_options text edit
        self.dlg.ui.dbOptionsText.setEnabled( True )
        self.dlg.ui.dbOptionsLbl.setEnabled( True )
        self.dlg.ui.pluginCombo.setEnabled( True )
        self.dlg.ui.pluginLbl.setEnabled( True )
        self.dlg.ui.stateLbl.setEnabled( True )
        self.dlg.ui.buildBtn.setEnabled( True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 1, True )

        # enable query tab only if the database if loaded
        if self.state >= 3:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 2, True )
        else:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 2, False )

    def onTabChanged( self, tab ):
        # options tab
        if tab == 1:
            plugin_arg = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ] }

            [status, msg] = self.wps.execute( 'get_option_descriptions', plugin_arg )
            self.wps.check_execute_response( [status, msg] )
            xml = ET.XML( msg )
            options = xml[2][0][2][0][0]

            [status, msg] = self.wps.execute( 'get_options', plugin_arg )
            self.wps.check_execute_response( [status, msg] )
            xml2 = ET.XML( msg )
            options_value = xml2[2][0][2][0][0]
            option_value = {}
            for option_val in options_value:
                option_value[ option_val.attrib['name'] ] = option_val.attrib['value']

            lay = self.dlg.ui.optionsLayout
            # clean the widget list
            rc = lay.rowCount()
            print "rc = ", rc
            if rc > 0:
                for row in range(0, rc):
                    print "remove", row
                    l1 = lay.itemAt( row, QFormLayout.LabelRole )
                    l2 = lay.itemAt( row, QFormLayout.FieldRole )
                    if l1 is None or l2 is None:
                        break
                    lay.removeItem( l1 )
                    lay.removeItem( l2 )
                    w1 = l1.widget()
                    w2 = l2.widget()
                    lay.removeWidget( w1 )
                    lay.removeWidget( w2 )
                    w1.close()
                    w2.close()

            row = 0
            for option in options:
                print option.attrib['name'], option.attrib['type'], option.attrib['description']
                lbl = QLabel( self.dlg )
                name = option.attrib['name'] + ''
                lbl.setText( name )
                lay.setWidget( row, QFormLayout.LabelRole, lbl )

                t = int(option.attrib['type'])

                val = option_value[name]
                # bool type
                if t == 0:
                    widget = QCheckBox( self.dlg )
                    if val == '1':
                        widget.setCheckState( Qt.Checked )
                    else:
                        widget.setCheckState( Qt.Unchecked )
                    QObject.connect(widget, SIGNAL("toggled(bool)"), lambda checked, name=name, t=t: self.onOptionChanged( name, t, checked ) )
                else:
                    widget = QLineEdit( self.dlg )
                    if t == 1:
                        valid = QIntValidator( widget )
                        widget.setValidator( valid )
                    if t == 2:
                        valid = QDoubleValidator( widget )
                        widget.setValidator( valid )
                    widget.setText( val )
                    QObject.connect(widget, SIGNAL("textChanged(const QString&)"), lambda text, name=name, t=t: self.onOptionChanged( name, t, text ) )
                lay.setWidget( row, QFormLayout.FieldRole, widget )

                row += 1

    def onOptionChanged( self, option_name, option_type, val ):
        print option_name, option_type, val
        # bool
        if option_type == 0:
            val = 1 if val else 0
        # int
        elif option_type == 1:
            val = int(val)
        elif option_type == 2:
            val = float(val)

        args = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ],
                 'options': [ True, ['options', ['option', {'name' : option_name, 'value' : str(val) } ] ] ] }

        [status, msg] = self.wps.execute( 'set_options', args )
        self.wps.check_execute_response( [status, msg] )

    # when the 'build' button get clicked
    def onBuildGraphs(self):
        # get the db options
        db_options = str(self.dlg.ui.dbOptionsText.text())

        [status, msg] = self.wps.execute( 'connect', { 'db_options' : [True, ['db_options', db_options ]] } )
        self.wps.check_execute_response( [status, msg] )
        [status, msg] = self.wps.execute( 'pre_build', {} )
        self.wps.check_execute_response( [status, msg] )
        [status, msg] = self.wps.execute( 'build', {} )
        self.wps.check_execute_response( [status, msg] )
        
    def onSelectionChanged(self, point, button):
        geom = QgsGeometry.fromPoint(point)
        p = geom.asPoint()
        print "Selection Changed", point.x(), point.y()
        self.canvas.unsetMapTool( self.clickTool )
        txt = "%f,%f" % (p.x(), p.y())
        if self.selectionType == 'origin':
            self.originPoint = p
            self.dlg.ui.originText.setText( txt )
            self.dlg.ui.originSelectBtn.setEnabled(True)
        else:
            self.destinationPoint = p
            self.dlg.ui.destinationText.setText( txt )
            self.dlg.ui.destinationSelectBtn.setEnabled(True)
        QObject.disconnect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onSelectionChanged)

    def onOriginSelect(self):
        self.selectionType = 'origin'
        self.dlg.ui.originSelectBtn.setEnabled(False)
        QObject.connect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onSelectionChanged)
        self.canvas.setMapTool(self.clickTool)

    def onDestinationSelect(self):
        self.selectionType = 'destination'
        self.dlg.ui.destinationSelectBtn.setEnabled(False)
        QObject.connect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onSelectionChanged)
        self.canvas.setMapTool(self.clickTool)

    def onCompute(self):
        ox = self.originPoint.x()
        oy = self.originPoint.y()
        dx = self.destinationPoint.x()
        dy = self.destinationPoint.y()
        criterion = self.dlg.criterion_id( str(self.dlg.ui.criterionList.currentText()) )
#        selected = self.dlg.ui.criterionList.selectedItems()
#        criteria = []
#        for it in self.dlg.ui.criterionList.selectedItems():
#            stdtxt = str(it.text())
#            criteria.append(self.dlg.criterion_id(stdtxt))
#
#        fmt_criteria = []
#        for criterion in criteria:
#            fmt_criteria.append(['optimizing_criterion', criterion])
        
        plugin_arg = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ] }

        args = plugin_arg
        args['request'] = [ True, ['request', 
                                   ['origin', ['x', str(ox)], ['y', str(oy)] ],
                                   ['departure_constraint', { 'type': 0, 'date_time': '2012-03-14T11:05:34' } ],
                                   ['optimizing_criterion', criterion ], 
                                   ['allowed_transport_types', 11 ],
                                   ['step',
                                    [ 'destination', ['x', str(dx)], ['y', str(dy)] ],
                                    [ 'constraint', { 'type' : 0, 'date_time':'2012-04-23T00:00:00' } ],
                                    [ 'private_vehicule_at_destination', 'true' ]
                                    ]
                                   ]
                            ]
        [status, msg] = self.wps.execute( 'pre_process', args )
        self.wps.check_execute_response( [status, msg] )

        [status, msg] = self.wps.execute( 'process', plugin_arg )
        self.wps.check_execute_response( [status, msg] )

        [status, msg] = self.wps.execute( 'result', plugin_arg )
        self.wps.check_execute_response( [status, msg] )

        xml_root = ET.XML( msg )
        overview_path =  xml_root[2][0][2][0][0][-1]
# create layer
        vl = QgsVectorLayer("LineString?crs=epsg:2154", "Itinerary", "memory")
        s = QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : '255,255,0'})
        vl.rendererV2().setSymbol(s)
        pr = vl.dataProvider()

# add a feature
        points = []
        for node in overview_path:
            x = float(node[0].text)
            y = float(node[1].text)
            points.append( QgsPoint( x, y ) )
        fet = QgsFeature()
        fet.setGeometry( QgsGeometry().fromPolyline( points ) )
        pr.addFeatures( [fet] )
        
        # update layer's extent when new features have been added
        # because change of extent in provider is not propagated to the layer
        vl.updateExtents()

        QgsMapLayerRegistry.instance().addMapLayer(vl)

    def unload(self):
        # Remove the plugin menu item and icon
        self.iface.removePluginMenu(u"&Compute route",self.action)
        self.iface.removeToolBarIcon(self.action)

    # run method that performs all the real work
    def run(self):
        # show the dialog
        self.dlg.show()
