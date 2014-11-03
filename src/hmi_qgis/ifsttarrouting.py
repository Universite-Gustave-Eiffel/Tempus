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

/***************************************************************************
 IfsttarRouting
                                 A QGIS plugin
 Get routing informations with various algorithms
                              -------------------
        begin                : 2012-04-12
        copyright            : (C) 2012-2013 by Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/
"""
import re
from wps_client import *
import config
import binascii
import os
import sys
import math
import random
import colorsys
import time
import signal
import pickle
from datetime import datetime

# Import the PyQt and QGIS libraries
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
#import PyQt4.QtCore
#import PyQt4.QtGui

from xml.etree import ElementTree as etree
XML_SCHEMA_VALIDATION = True
try:
        from lxml import etree as ET
except ImportError:
        XML_SCHEMA_VALIDATION = False

# Initialize Qt resources from file resources.py
import resources_rc
# Import the code for the dialog
from ifsttarroutingdock import IfsttarRoutingDock
# Import the splash screen
from ui_splash_screen import Ui_SplashScreen

from history_file import ZipHistoryFile
from result_selection import ResultSelection
from altitude_profile import AltitudeProfile
from wkb import WKB
import tempus_request as Tempus
from polygon_selection import PolygonSelection
from psql_helper import psql_query
from consolelauncher import ConsoleLauncher

HISTORY_FILE = os.path.expanduser('~/.ifsttarrouting.db')
PREFS_FILE = os.path.expanduser('~/.ifsttarrouting.prefs')

ROADMAP_LAYER_NAME = "Tempus_Roadmap_"
TRACE_LAYER_NAME = "Tempus_Trace_"

# There has been an API change regarding vector layer on 1.9 branch
NEW_API = 'commitChanges' in dir(QgsVectorLayer)

#
# clears a FormLayout
def clearLayout( lay ):
    # clean the widget list
    rc = lay.rowCount()
    if rc > 0:
        for row in range(0, rc):
            l1 = lay.itemAt( rc-row-1, QFormLayout.LabelRole )
            l2 = lay.itemAt( rc-row-1, QFormLayout.FieldRole )
            if l1 is not None:
                lay.removeItem( l1 )
                w1 = l1.widget()
                lay.removeWidget( w1 )
                w1.close()
            if l2 is not None:
                lay.removeItem( l2 )
                w2 = l2.widget()
                lay.removeWidget( w2 )
                w2.close()

#
# clears a BoxLayout
def clearBoxLayout( lay ):
    rc = lay.count()
    if rc == 0:
        return
    for row in range(0, rc):
        # remove in reverse
        item = lay.itemAt(rc - row - 1)
        lay.removeItem(item)
        w = item.widget()
        lay.removeWidget(w)
        w.close()

# minutes (float) to hh:mm:ss
def min2hm( m ):
        hh = int(m/60)
        mm = int(m-hh*60)
        ss = int((m-hh*60-mm) * 60)
        return "%02d:%02d:%02d" % (hh,mm,ss)

class SplashScreen(QDialog):
    def __init__(self):
        QDialog.__init__(self)
        self.ui = Ui_SplashScreen()
        self.ui.setupUi(self)

# process error message returned by WPS
def displayError( error ):
    if error.startswith('<ows:ExceptionReport'):
        # WPS exception
        t = ET.fromstring( error )
        QMessageBox.warning( None, "Tempus error", t[0][0].text )
    else:
        # HTML
        QMessageBox.warning( None, "Error", error )

class IfsttarRouting:

    def __init__(self, iface):
        # Save reference to the QGIS interface
        self.iface = iface
        # Create the dialog and keep reference
        self.canvas = self.iface.mapCanvas()
        self.dlg = IfsttarRoutingDock( self.canvas )
        if os.path.exists( PREFS_FILE ):
            f = open( PREFS_FILE, 'r' )
            prefs = pickle.load( f )
            self.dlg.loadState( prefs['query'] )

        # initialize plugin directory
        self.plugin_dir = QFileInfo(QgsApplication.qgisUserDbFilePath()).path() + "/python/plugins/ifsttarrouting"
        # initialize locale
        localePath = ""
        locale = QSettings().value("locale/userLocale")[0:2]
       
        if QFileInfo(self.plugin_dir).exists():
            localePath = self.plugin_dir + "/i18n/ifsttarrouting_" + locale + ".qm"

        if QFileInfo(localePath).exists():
            self.translator = QTranslator()
            self.translator.load(localePath)

            if qVersion() > '4.3.3':
                QCoreApplication.installTranslator(self.translator)

        self.wps = None
        self.historyFile = ZipHistoryFile( HISTORY_FILE )
        self.dlg.ui.historyFileLbl.setText( HISTORY_FILE )

        # list of transport types
        # array of Tempus.TransportType
        self.transport_modes = []
        self.transport_modes_dict = {}
        # list of public networks
        # array of Tempus.TransportNetwork
        self.networks = []

        # plugin descriptions
        # dict plugin_name =>  Tempus.Plugin
        self.plugins = {}
        # where to store plugin options
        # dict plugin_name => Tempus.OptionValue
        self.plugin_options = {}

        self.currentRoadmap = None

        # prevent reentrance when roadmap selection is in progress
        self.selectionInProgress = False

        # profile widget
        self.profile = None

        self.setStateText("DISCONNECTED")

        self.server = None

    def initGui(self):
        # Create action that will start plugin configuration
        self.action = QAction(QIcon(":/plugins/ifsttarrouting/icon.png"), \
            u"Compute route", self.iface.mainWindow())
        # connect the action to the run method
        QObject.connect(self.action, SIGNAL("triggered()"), self.run)

        QObject.connect(self.dlg.ui.connectBtn, SIGNAL("clicked()"), self.onConnect)
        self.dlg.ui.tempusBinPathBrowse.clicked.connect( self.onBinPathBrowse )
        self.dlg.ui.addPluginToLoadBtn.clicked.connect( self.onAddPluginToLoad )
        self.dlg.ui.removePluginToLoadBtn.clicked.connect( self.onRemovePluginToLoad )
        self.dlg.ui.startServerBtn.clicked.connect( self.onStartServer )
        self.dlg.ui.stopServerBtn.clicked.connect( self.onStopServer )

        QObject.connect(self.dlg.ui.computeBtn, SIGNAL("clicked()"), self.onCompute)
        QObject.connect(self.dlg.ui.resetBtn, SIGNAL("clicked()"), self.onReset)
        QObject.connect(self.dlg.ui.verticalTabWidget, SIGNAL("currentChanged( int )"), self.onTabChanged)

        QObject.connect( self.dlg.ui.pluginCombo, SIGNAL("currentIndexChanged(int)"), self.update_plugin_options )

        # double click on a history's item
        QObject.connect( self.dlg.ui.historyList, SIGNAL("itemDoubleClicked(QListWidgetItem *)"), self.onHistoryItemSelect )
        
        # click on the 'reset' button in history tab
        QObject.connect( self.dlg.ui.reloadHistoryBtn, SIGNAL("clicked()"), self.loadHistory )
        QObject.connect( self.dlg.ui.deleteHistoryBtn, SIGNAL("clicked()"), self.onDeleteHistoryItem )
        QObject.connect( self.dlg.ui.importHistoryBtn, SIGNAL("clicked()"), self.onImportHistory )

        # click on a result radio button
        QObject.connect(ResultSelection.buttonGroup, SIGNAL("buttonClicked(int)"), self.onResultSelected )

        QObject.connect( self.dlg.ui.roadmapTable, SIGNAL("itemSelectionChanged()"), self.onRoadmapSelectionChanged )

        # show elevations button
        QObject.connect( self.dlg.ui.showElevationsBtn, SIGNAL("clicked()"), self.onShowElevations )

        self.originPoint = QgsPoint()
        self.destinationPoint = QgsPoint()

        # Add toolbar button and menu item
        self.iface.addToolBarIcon(self.action)
        self.clipAction = QAction( u"Polygon subset (from mouse)", self.iface.mainWindow())
        self.clipAction.triggered.connect( self.onClip )
        self.clipActionFromSel = QAction( u"Polygon subset from selection", self.iface.mainWindow())
        self.clipActionFromSel.triggered.connect( self.onClipFromSel )
        self.loadLayersAction = QAction( u"Load layers", self.iface.mainWindow())
        self.loadLayersAction.triggered.connect( self.onLoadLayers )
        self.exportTraceAction = QAction( u"Export trace", self.iface.mainWindow())
        self.exportTraceAction.triggered.connect( self.onExportTrace )

        self.iface.addPluginToMenu(u"&Tempus", self.action)
        self.iface.addPluginToMenu(u"&Tempus", self.clipAction)
        self.iface.addPluginToMenu(u"&Tempus", self.clipActionFromSel)
        self.iface.addPluginToMenu(u"&Tempus", self.loadLayersAction)
        self.iface.addPluginToMenu(u"&Tempus", self.exportTraceAction)
        self.iface.addDockWidget( Qt.LeftDockWidgetArea, self.dlg )

        # init the history
        self.loadHistory()

        # init server configuration from settings
        s = QSettings()
        self.dlg.ui.tempusBinPathEdit.setText( s.value("IfsttarTempus/startTempus", "/usr/local/bin/startTempus.sh" ) )
        self.dlg.ui.dbOptionsEdit.setText( s.value("IfsttarTempus/dbOptions", "dbname=tempus_test_db user=postgres" ) )
        self.dlg.ui.schemaNameEdit.setText( s.value("IfsttarTempus/schemaName", "tempus" ) )
        plugins = s.value("IfsttarTempus/plugins", ["sample_road_plugin", "sample_multi_plugin", "dynamic_multi_plugin"] )
        for p in plugins:
            item = QListWidgetItem( p )
            item.setFlags( item.flags() | Qt.ItemIsEditable )
            self.dlg.ui.pluginsToLoadList.insertItem(0, item )

        self.clear()

    # reset widgets contents and visibility
    # as if it was just started
    def clear( self ):
        self.dlg.ui.verticalTabWidget.setTabEnabled( 1, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 2, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 3, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 4, False )        
        self.dlg.ui.showElevationsBtn.hide()
        clearLayout( self.dlg.ui.resultLayout )
        self.dlg.ui.roadmapTable.clear()
        self.dlg.ui.roadmapTable.setRowCount(0)
        self.dlg.ui.roadmapTable.setHorizontalHeaderLabels( ["", "Direction", "Costs"] )

    def setStateText( self, text ):
        self.dlg.setWindowTitle( "Routing - " + text + "" )

    def get_pgsql_connection( self ):
        dlg = QInputDialog( None )

        q = QSettings()
        q.beginGroup("/PostgreSQL/connections")
        items = q.childGroups()
        current = items.index(q.value('selected'))
        connection, ok = QInputDialog.getItem(None, "Db connection", "Database to connect to", items, current, False )
        if not ok:
            return
        q.beginGroup(connection)
        db_params = "dbname='%s'" % q.value("database")
        host = q.value("host")
        if host:
            db_params += " host='%s'" % host
        port = q.value("port")
        if port:
            db_params += " port=%s" % port
        user = q.value("username")
        if user:
            db_params += " user='%s'" % user
        passwd = q.value("password")
        if passwd:
            db_params += " password='%s'" % passwd
        return db_params

    def onLoadLayers( self ):
        cwd = os.path.dirname(os.path.abspath(__file__))
        project_tmpl = cwd + '/tempus.qgs.tmpl'
        project = cwd + '/tempus.qgs'

        db_params = self.get_pgsql_connection()
        if db_params is None:
            return

        # FIXME better use psycopg2 here ?
        (ll,_) = psql_query( db_params, "SELECT schema_name FROM tempus.subset" )
        items = ['tempus'] + [ n[1:] for n in ll.split('\n') ]
        schema, ok = QInputDialog.getItem(None, "Schema", "Schema to load", items, 0, False )
        if not ok:
            return

        fo = open(project, 'w+')
        with open(project_tmpl) as f:
            s = f.read()
            s = s.replace('%DB_PARAMS%', str(db_params))
            s = s.replace('%SCHEMA%', str(schema))
            fo.write( s )
            fo.close()
        QgsMapLayerRegistry.instance().removeAllMapLayers()
        ok = QgsProject.instance().read( QFileInfo( project ) )
        if not ok:
            QMessageBox.warning( None, "Project error", QgsProject.instance().error())

    def onExportTrace( self ):
        # look for the trace layer
        maps = QgsMapLayerRegistry.instance().mapLayers()
        trace_layer = None
        for k,v in maps.items():
            if v.name().startswith(TRACE_LAYER_NAME):
                trace_layer = v
                break
        if v is None:
            QMessageBox.warning( None, "No trace layer", "Cannot find a trace layer to export !")
            return

        # ask for a filename
        fn = QFileDialog.getSaveFileName( self.dlg, "GraphViz file where to export to", "", "GraphViz file (*.dot)" )
        if not fn:
            return

        nodes = {}
        edges = {}
        d = trace_layer.dataProvider()
        attr_names = [ f.name() for f in d.fields() if f.name() not in ('origin', 'destination') ]
        for f in d.getFeatures():
            attr = f.fields()
            o = f['origin']
            d = f['destination']
            g = f.geometry().asPolyline()
            nodes[ o ] = g[0]
            nodes[ d ] = g[-1]
            edge_attr = {}
            edges[ '%s->%s' % (o,d) ] = dict([(k, f[k]) for k in attr_names])

        with open(fn, 'w+') as f:
            f.write("digraph G {\n")
            for n,p in nodes.iteritems():
                f.write('%s[pos="%f,%f"];\n' % (n, p[0], p[1]))
            for n,attrs in edges.iteritems():
                attr_str = ' '.join(['%s=%s' % (k,v) for k,v in attrs.iteritems()])
                f.write(n)
                f.write('[label="' + attr_str + '"]\n')
            f.write('}\n')

    def onClip( self ):
        # select a database
        db_params = self.get_pgsql_connection()
        if db_params is None:
            return

        QMessageBox.information( None, "Polygon selection",
                                     "Please select the polygon used for subsetting. Cancel with right click. Confirm the last point with a double click.")
        self.select = PolygonSelection( self.canvas )
        self.select.polygonCaptured.connect( lambda valid : self.onPolygonCaptured(valid, db_params) )
        self.canvas.setMapTool( self.select )

    def onClipFromSel( self ):
        layer = self.iface.activeLayer()
        if not isinstance(layer, QgsVectorLayer):
            QMessageBox.critical( None, "Polygon selection",
                                  "No vector layer selected" )
            return
        if len(layer.selectedFeatures()) == 0:
            QMessageBox.critical( None, "Polygon selection",
                                  "No polygon selected")
            return
        if len(layer.selectedFeatures()) > 1:
            QMessageBox.critical( None, "Polygon selection",
                                  "More than one polygon selected" )
            return
        feature = layer.selectedFeatures()[0]
        if feature.geometry() is None or feature.geometry().type() != QGis.Polygon:
            QMessageBox.critical( None, "Polygon selection",
                                  "No polygon selected" )
            return

        geom = QgsGeometry(feature.geometry())
        trans = QgsCoordinateTransform(layer.crs(), QgsCoordinateReferenceSystem( "EPSG:2154" ) )
        geom.transform( trans )
        polygon_wkt = geom.exportToWkt()

        # select a database
        db_params = self.get_pgsql_connection()
        if db_params is None:
            return

        self.doPolygonSubset( polygon_wkt, db_params )

    def onPolygonCaptured( self, valid, db_params ):
        self.canvas.unsetMapTool( self.select )
        if not valid:
            return
        wkt = self.select.polygon
        self.doPolygonSubset( wkt, db_params )

    def doPolygonSubset( self, wkt, db_params ):
        schema, ok = QInputDialog.getText(None, "Name of the subset to create (schema)", "Schema name")

        sql = "SELECT tempus.create_subset('%s','SRID=2154;%s');" % (schema, wkt)

        dlg = QDialog(self.dlg)
        v = QVBoxLayout()
        log = QLabel( dlg )
        v.addWidget(log)
        btn = QDialogButtonBox( dlg )
        btn.addButton(QDialogButtonBox.Ok)
        v.addWidget(btn)
        dlg.setLayout(v)
        log.setText('Executing script ...\n')
        dlg.show()

        QCoreApplication.processEvents()

        (sout, serr) = psql_query( db_params, sql )

        btn.accepted.connect( dlg.close )
        log.setText('Outputs:\n' + sout + 'Errors:\n' + serr)
        dlg.setModal( True )

    def onBinPathBrowse( self ):
        d = QFileDialog.getOpenFileName( self.dlg, "Tempus wps executable path" )
        if d is not None:
            self.dlg.ui.tempusBinPathEdit.setText( d )

    def onAddPluginToLoad( self ):
        item = QListWidgetItem("new_plugin")
        item.setFlags( item.flags() | Qt.ItemIsEditable)
        self.dlg.ui.pluginsToLoadList.insertItem(0,item)

    def onRemovePluginToLoad( self ):
        r = self.dlg.ui.pluginsToLoadList.currentRow()
        if r != -1:
            self.dlg.ui.pluginsToLoadList.takeItem( r )

    def onStartServer( self ):
        self.onStopServer()
        executable = self.dlg.ui.tempusBinPathEdit.text()
        dbOptions = self.dlg.ui.dbOptionsEdit.text()
        schemaName = self.dlg.ui.schemaNameEdit.text()
        plugins = []
        for i in range(self.dlg.ui.pluginsToLoadList.count()):
            plugins.append( self.dlg.ui.pluginsToLoadList.item(i).text() )
        cmdLine = [executable]
        cmdLine += ['-s', schemaName]
        for p in plugins:
            cmdLine += ['-l', p]
        if dbOptions != '':
            cmdLine += ['-d', dbOptions ]

        self.server = ConsoleLauncher( cmdLine )
        self.server.launch()

        # save parameters
        s = QSettings()
        s.setValue("IfsttarTempus/startTempus", executable )
        s.setValue("IfsttarTempus/dbOptions", dbOptions )
        s.setValue("IfsttarTempus/schemaName", schemaName )
        s.setValue("IfsttarTempus/plugins", plugins )

    def onStopServer( self ):
        if self.server is not None:
            self.server.stop()

    # when the 'connect' button gets clicked
    def onConnect( self ):
        try:
            self.wps = Tempus.TempusRequest( self.dlg.ui.wpsUrlText.text() )
        except RuntimeError as e:
            displayError( e.args[0] )
            return

        self.getPluginList()
        if len(self.plugins) == 0:
            QMessageBox.warning( self.dlg, "Warning", "No plugin loaded server side, no computation could be done" )
        else:
            # initialize plugin option values
            self.initPluginOptions()
            self.displayPlugins( self.plugins )

            self.getConstants()
            self.displayTransportAndNetworks()
        
            self.dlg.ui.pluginCombo.setEnabled( True )
            self.dlg.ui.verticalTabWidget.setTabEnabled( 1, True )
            self.dlg.ui.verticalTabWidget.setTabEnabled( 2, True )
            self.dlg.ui.verticalTabWidget.setTabEnabled( 3, True )
            self.dlg.ui.verticalTabWidget.setTabEnabled( 4, True )
            self.dlg.ui.computeBtn.setEnabled( True )
            self.setStateText("connected")

        # restore the history to HISTORY_FILE, if needed (after import)
        if self.historyFile.filename != HISTORY_FILE:
            self.historyFile = ZipHistoryFile( HISTORY_FILE )
            self.dlg.ui.historyFileLbl.setText( HISTORY_FILE )
            self.loadHistory()

    def getPluginList( self ):
        # get plugin list
        try:
            plugins = self.wps.plugin_list()
        except RuntimeError as e:
            displayError( e.args[1] )
            return
        self.plugins.clear()
        for plugin in plugins:
            self.plugins[plugin.name] = plugin

    def initPluginOptions( self ):
        self.plugin_options.clear()
        for name, plugin in self.plugins.iteritems():
            optval = {}
            for k,v in plugin.options.iteritems():
                optval[k] = v.default_value.value
            self.plugin_options[name] = optval

    def getConstants( self ):
        if self.wps is None:
            return
        try:
            (transport_modes, transport_networks) = self.wps.constant_list()
        except RuntimeError as e:
            displayError( e.args[1] )
            return
        self.transport_modes = transport_modes
        self.transport_modes_dict = {}
        for t in self.transport_modes:
                self.transport_modes_dict[t.id] = t

        self.networks = transport_networks

    def update_plugin_options( self, plugin_idx ):
        if plugin_idx == -1:
            return

        plugin_name = str(self.dlg.ui.pluginCombo.currentText())
        self.displayPluginOptions( plugin_name )

    def onTabChanged( self, tab ):
        # Plugin tab
        if tab == 1:
            self.update_plugin_options( 0 )

        # 'Query' tab
        elif tab == 2:
                # prepare for a new query
#            self.dlg.reset()
            self.dlg.inQuery()

    def loadHistory( self ):
        #
        # History tab
        #
        self.dlg.ui.historyList.clear()
        r = self.historyFile.getRecordsSummary()
        for record in r:
            id = record[0]
            date = record[1]
            dt = datetime.strptime( date, "%Y-%m-%dT%H:%M:%S.%f" )
            item = QListWidgetItem()
            item.setText(dt.strftime( "%Y-%m-%d at %H:%M:%S" ))
            item.setData( Qt.UserRole, int(id) )
            self.dlg.ui.historyList.addItem( item )

    def onDeleteHistoryItem( self ):
        msgBox = QMessageBox()
        msgBox.setIcon( QMessageBox.Warning )
        msgBox.setText( "Warning" )
        msgBox.setInformativeText( "Are you sure you want to remove these items from the history ?" )
        msgBox.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        ret = msgBox.exec_()
        if ret == QMessageBox.No:
            return

        sel = self.dlg.ui.historyList.selectedIndexes()
        for item in sel:
            id = item.data( Qt.UserRole )
            self.historyFile.removeRecord( id )
        # reload
        self.loadHistory()

    def onOptionChanged( self, plugin_name, option_name, option_type, val ):
            if option_type == Tempus.OptionType.Bool:
                    val = True if val else False
            elif option_type == Tempus.OptionType.Int:
                    # int
                    if val:
                            val = int(val)
                    else:
                            val = 0
            elif option_type == Tempus.OptionType.Float:
                    # float
                    if val:
                            val = float(val)
                    else:
                            val = 0.0
            self.plugin_options[plugin_name][option_name] = val

    #
    # Take a XML tree from the WPS 'result' operation
    # add an 'Itinerary' layer on the map
    #
    def displayRoadmapLayer(self, steps, lid):
        #
        # create layer

        lname = "%s%d" % (ROADMAP_LAYER_NAME, lid)
        # create a new vector layer
        vl = QgsVectorLayer("LineString?crs=epsg:2154", lname, "memory")

        pr = vl.dataProvider()

        if NEW_API:
            vl.startEditing()
            vl.addAttribute( QgsField( "transport_mode", QVariant.Int ) )
        else:
            pr.addAttributes( [ QgsField( "transport_mode", QVariant.Int ) ] )


        root_rule = QgsRuleBasedRendererV2.Rule( QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : '255,255,0'} ))

        for mode_id, mode in self.transport_modes_dict.iteritems():
                p = float(mode_id-1) / len(self.transport_modes_dict)
                rgb = colorsys.hsv_to_rgb( p, 0.7, 0.9 )
                color = '%d,%d,%d' % (rgb[0]*255, rgb[1]*255, rgb[2]*255)
                rule = QgsRuleBasedRendererV2.Rule( QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : color}),
                                                    0,
                                                    0,
                                                    "transport_mode=%d" % mode_id,
                                                    mode.name)
                root_rule.appendChild( rule )

        renderer = QgsRuleBasedRendererV2( root_rule );
        vl.setRendererV2( renderer )

        # browse steps
        for step in steps:
            fet = QgsFeature()
            if step.wkb != '':
                # find wkb geometry
                wkb = WKB(step.wkb)
                wkb = wkb.force2d()
                geo = QgsGeometry()
                geo.fromWkb( binascii.unhexlify(wkb) )
                fet.setGeometry( geo )
            else:
                geo = QgsGeometry.fromWkt('POINT(0 0)')
                fet.setGeometry(geo)

            if isinstance(step, Tempus.TransferStep):
                mode = step.final_mode
            else:
                mode = step.mode
            if NEW_API:
                fet.setAttributes( [ mode ] )
            else:
                fet.setAttributeMap( { 0: mode } )
            pr.addFeatures( [fet] )

        if NEW_API:
            vl.commitChanges()
        else:
            # We MUST call this manually (see http://hub.qgis.org/issues/4687)
            vl.updateFieldMap()
        # update layer's extent when new features have been added
        # because change of extent in provider is not propagated to the layer
        vl.updateExtents()

        QgsMapLayerRegistry.instance().addMapLayers( [vl] )
        self.selectRoadmapLayer( lid )

        # connect selection change signal
        QObject.connect( vl, SIGNAL("selectionChanged()"), lambda layer=vl: self.onLayerSelectionChanged(layer) )

    def displayTrace(self, trace, lid, vl):

        # first pass to get the list of variants
        variants_type = {}
        for ve in trace:
            for k,v in ve.variants.iteritems():
                if variants_type.get(k) is None:
                    variants_type[k] = v.__class__

        if vl is None:
            lname = "%s%d" % ( TRACE_LAYER_NAME, lid )
            # create a new vector layer
            vl = QgsVectorLayer("LineString?crs=epsg:2154", lname, "memory")
            QgsMapLayerRegistry.instance().addMapLayers( [vl] )

        pr = vl.dataProvider()

        vl.startEditing()
        vl.addAttribute( QgsField( "type", QVariant.Int ) )
        vl.addAttribute( QgsField( "origin", QVariant.Int ) )
        vl.addAttribute( QgsField( "destination", QVariant.Int ) )
        for n, t in variants_type.iteritems():
            if t == int:
                vt = QVariant.Int
            elif t == float:
                vt = QVariant.Double
            elif t == str:
                vt = QVariant.String
            elif t == bool:
                vt = QVariant.Bool
            vl.addAttribute( QgsField( n, vt ) )

        for ve in trace:
            fet = QgsFeature()
            if ve.wkb != '':
                # find wkb geometry
                wkb = WKB(ve.wkb)
                wkb = wkb.force2d()
                geo = QgsGeometry()
                geo.fromWkb( binascii.unhexlify(wkb) )
                fet.setGeometry( geo )
            else:
                geo = QgsGeometry.fromWkt('POINT(0 0)')
                fet.setGeometry(geo)

            if isinstance(ve.origin, Tempus.RoadVertex) and isinstance(ve.destination, Tempus.RoadVertex):
                type = Tempus.ConnectionType.Road2Road
            elif isinstance(ve.origin, Tempus.RoadVertex) and isinstance(ve.destination, Tempus.PtVertex):
                type = Tempus.ConnectionType.Road2Transport
            elif isinstance(ve.origin, Tempus.PtVertex) and isinstance(ve.destination, Tempus.RoadVertex):
                type = Tempus.ConnectionType.Transport2Road
            elif isinstance(ve.origin, Tempus.PtVertex) and isinstance(ve.destination, Tempus.PtVertex):
                type = Tempus.ConnectionType.Transport2Transport
            elif isinstance(ve.origin, Tempus.RoadVertex) and isinstance(ve.destination, Tempus.PoiVertex):
                type = Tempus.ConnectionType.Road2Poi
            elif isinstance(ve.origin, Tempus.PoiVertex) and isinstance(ve.destination, Tempus.RoadVertex):
                type = Tempus.ConnectionType.Poi2Road
            attrs = [ type, ve.origin.id, ve.destination.id ]
            attrs += [ve.variants.get(k) for k in ve.variants.keys()]
            fet.setAttributes( attrs )
            pr.addFeatures( [fet] )

        vl.commitChanges()
        vl.updateExtents()

    #
    # Select the roadmap layer
    #
    def selectRoadmapLayer( self, id ):
        legend = self.iface.legendInterface()
        lname = "%s%d" % (ROADMAP_LAYER_NAME, id)
        maps = QgsMapLayerRegistry.instance().mapLayers()
        for k,v in maps.items():
            if v.name()[0:len(ROADMAP_LAYER_NAME)] == ROADMAP_LAYER_NAME:
                if v.name() == lname:
                    legend.setLayerVisible( v, True )
                else:
                    legend.setLayerVisible( v, False )
    #
    # Take a XML tree from the WPS 'result' operation
    # and fill the "roadmap" tab
    #
    def displayRoadmapTab( self, result ):
        last_movement = 0
        roadmap = result.steps
        row = 0
        self.dlg.ui.roadmapTable.clear()
        self.dlg.ui.roadmapTable.setRowCount(0)
        self.dlg.ui.roadmapTable.setHorizontalHeaderLabels( ["", "", "Direction"] )

        # array of altitudes (distance, elevation)

        # get or create the graphics scene
        w = self.dlg.ui.resultSelectionLayout
        lastWidget = w.itemAt( w.count() - 1 ).widget()
        if isinstance(lastWidget, AltitudeProfile):
            self.profile = lastWidget
        else:
            self.profile = AltitudeProfile( self.dlg )
            self.profile.hide()
            self.dlg.ui.resultSelectionLayout.addWidget( self.profile )

        self.profile.clear()

        current_time = result.starting_date_time.hour*60 + result.starting_date_time.minute + result.starting_date_time.second / 60.0;

        first = True
        leaving_time = None

        for step in roadmap:
            if first:
                if isinstance(step, Tempus.TransferStep ) and step.poi == '':
                    from_private_parking = True
                    text = ''
                else:
                    from_private_parking = False
                    text = "Initial mode: %s<br/>\n" % self.transport_modes_dict[step.mode].name
                first = False
            else:
                text = ''
            icon_text = ''
            cost_text = ''

            if step.wkb and step.wkb != '':
                wkb = WKB(step.wkb)
                pts = wkb.dumpPoints()
                if len(pts) > 0: # if 3D points ...
                        prev = pts[0]
                        for p in pts[1:]:
                                dist = math.sqrt((p[0]-prev[0])**2 + (p[1]-prev[1])**2)
                                self.profile.addElevation( dist, prev[2], p[2], row )
                                prev = p

            if isinstance(step, Tempus.RoadStep):
                road_name = step.road
                movement = step.end_movement
                text += "<p>"
                action_txt = 'Continue on '
                if last_movement == Tempus.EndMovement.TurnLeft:
                    icon_text += "<img src=\"%s/turn_left.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                    action_txt = "Turn left on "
                elif last_movement == Tempus.EndMovement.TurnRight:
                    icon_text += "<img src=\"%s/turn_right.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                    action_txt = "Turn right on "
                elif last_movement >= Tempus.EndMovement.RoundAboutEnter and last_movement < Tempus.EndMovement.YouAreArrived:
                    icon_text += "<img src=\"%s/roundabout.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                text += action_txt + road_name + "<br/>\n"
                text += "</p>"
                last_movement = movement

            elif isinstance(step, Tempus.PublicTransportStep ):
                # set network text as icon
                icon_text = step.network
                if step.wait_time > 0.0:
                        text += "Wait %s <br/>" % min2hm(step.wait_time)
                text += "At %s<br/>\n" % min2hm(step.departure_time+step.wait_time)
                pt_name = self.transport_modes_dict[step.mode].name
                text += "%s Take the %s %s from '%s' to '%s'" % (step.trip_id, pt_name, step.route, step.departure, step.arrival)
                leaving_time = step.arrival_time

            elif isinstance(step, Tempus.RoadTransportStep ):
                icon_text = step.network
                if step.type == Tempus.ConnectionType.Road2Transport:
                    text += "Go to the '%s' station from %s" % (step.stop, step.road)
                elif step.type == Tempus.ConnectionType.Transport2Road:
                        if leaving_time is not None:
                                text += "At %s<br/>\n" % min2hm(leaving_time)
                        text += "Leave the '%s' station to %s" % (step.stop, step.road)
                else:
                    text += "Connection between '%s' and '%s'" % (step.stop, step.road)

            elif isinstance(step, Tempus.TransferStep ):
                if from_private_parking:
                    # my private parking
                    text += "Take your %s<br/>\n" % self.transport_modes_dict[step.final_mode].name
                    text += "Go on %s" % step.road
                    from_private_parking = False
                else:
                    text += "On %s,<br/>At %s:<br/>\n" % (step.road, step.poi)
                    if step.mode != step.final_mode:
                            imode = self.transport_modes_dict[step.mode]
                            fmode = self.transport_modes_dict[step.final_mode]
                            add_t = []
                            if imode.need_parking:
                                    add_t += ["park your %s" % imode.name]
                            add_t += ["take a %s" % fmode.name]
                            text += ' and '.join(add_t)
                    else:
                            text = "Continue on %s" % step.road

            for k,v in step.costs.iteritems():
                if k == Tempus.Cost.Duration:                    
                    cost_text += "At %s<br/>\n" % min2hm(current_time)
                    current_time += v
                    cost_text += "Duration " + min2hm(v)
                else:
                    cost_text += "%s: %.1f %s<br/>\n" % (Tempus.CostName[k], v, Tempus.CostUnit[k])
                
            self.dlg.ui.roadmapTable.insertRow( row )
            descLbl = QLabel()
            descLbl.setText( text )
            descLbl.setMargin( 5 )
            self.dlg.ui.roadmapTable.setCellWidget( row, 2, descLbl )

            if icon_text != '':
                lbl = QLabel()
                lbl.setText( icon_text )
                lbl.setMargin(5)
                self.dlg.ui.roadmapTable.setCellWidget( row, 1, lbl )

            costText = QLabel()
            costText.setText( cost_text )
            costText.setMargin( 5 )
            self.dlg.ui.roadmapTable.setCellWidget( row, 0, costText )
            self.dlg.ui.roadmapTable.resizeRowToContents( row )
            row += 1

        # Adjust column widths
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(0)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 0, w )
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(1)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 1, w )
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(2)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 2, w )
        self.dlg.ui.roadmapTable.horizontalHeader().setStretchLastSection( True )

        if not self.profile.empty():
            self.dlg.ui.showElevationsBtn.setEnabled(True)
            self.profile.displayElevations()
        else:
            self.dlg.ui.showElevationsBtn.setEnabled(False)
    #
    # Take a XML tree from the WPS 'metrics' operation
    # and fill the 'Metrics' tab
    #
    def displayMetrics( self, metrics ):
        row = 0
        clearLayout( self.dlg.ui.resultLayout )

        for name, metric in metrics.iteritems():
            lay = self.dlg.ui.resultLayout
            lbl = QLabel( self.dlg )
            lbl.setText( name )
            lay.setWidget( row, QFormLayout.LabelRole, lbl )
            widget = QLineEdit( self.dlg )
            widget.setText( str(metric) )
            widget.setEnabled( False )
            lay.setWidget( row, QFormLayout.FieldRole, widget )
            row += 1

    #
    # Take XML trees of 'plugins' and an index of selection
    # and fill the 'plugin' tab
    #
    def displayPlugins( self, plugins ):
        self.dlg.ui.pluginCombo.clear()
        self.options_desc = {}
        for name, plugin in plugins.iteritems():
            self.dlg.ui.pluginCombo.insertItem(0, name )

    #
    # Take XML tree of 'options' and a dict 'option_values'
    # and fill the options part of the 'plugin' tab
    #
    def displayPluginOptions( self, plugin_name ):
        lay = self.dlg.ui.optionsLayout
        clearLayout( lay )

        row = 0
        for name, option in self.plugins[plugin_name].options.iteritems():
            lbl = QLabel( self.dlg )
            lbl.setText( option.description )
            lay.setWidget( row, QFormLayout.LabelRole, lbl )
            
            t = option.type()
            
            val = self.plugin_options[plugin_name][name]
            if t == Tempus.OptionType.Bool:
                widget = QCheckBox( self.dlg )
                if val == True:
                    widget.setCheckState( Qt.Checked )
                else:
                    widget.setCheckState( Qt.Unchecked )
                QObject.connect(widget, SIGNAL("toggled(bool)"), lambda checked, name=name, t=t, pname=plugin_name: self.onOptionChanged( pname, name, t, checked ) )
            else:
                widget = QLineEdit( self.dlg )
                if t == Tempus.OptionType.Int:
                    valid = QIntValidator( widget )
                    widget.setValidator( valid )
                if t == Tempus.OptionType.Float:
                    valid = QDoubleValidator( widget )
                    widget.setValidator( valid )
                widget.setText( str(val) )
                QObject.connect(widget, SIGNAL("textChanged(const QString&)"), lambda text, name=name, t=t, pname=plugin_name: self.onOptionChanged( pname, name, t, text ) )
            lay.setWidget( row, QFormLayout.FieldRole, widget )
            
            row += 1

        self.dlg.set_supported_criteria( self.plugins[plugin_name].supported_criteria )
        self.dlg.set_intermediate_steps_support( self.plugins[plugin_name].intermediate_steps )
        self.dlg.set_depart_after_support( self.plugins[plugin_name].depart_after )
        self.dlg.set_arrive_before_support( self.plugins[plugin_name].arrive_before )

    #
    # Take an XML trees from 'constant_list'
    # load them and display them
    def displayTransportAndNetworks( self ):

        listModel = QStandardItemModel()
        for ttype in self.transport_modes:
            item = QStandardItem( ttype.name )
            item.setFlags(Qt.ItemIsUserCheckable | Qt.ItemIsEnabled)
            if ttype.id != 1:
                    item.setData( Qt.Unchecked, Qt.CheckStateRole)
            else:
                    item.setData( Qt.Checked, Qt.CheckStateRole )
            item.setData( ttype.id, Qt.UserRole )
            listModel.appendRow(item)

        def on_transport_changed( item ):
            t = self.dlg.selected_transports()
            c = 1 in t or 2 in t # private cycle or car
            self.dlg.parkingEnabledBox.setEnabled( c )
            self.dlg.parkingChooser.setEnabled( c and self.dlg.parkingEnabledBox.checkState() == Qt.Checked )

        listModel.itemChanged.connect( on_transport_changed )
        self.dlg.ui.transportList.setModel( listModel )
        on_transport_changed( item )

    #
    # Take an XML tree from the WPS 'results'
    # and load/display them
    #
    def displayResults( self, results ):
        # result radio button id
        self.result_ids=[]
        # clear the vbox layout
        clearBoxLayout( self.dlg.ui.resultSelectionLayout )
        k = 1
        for result in results:
            start = "%02d:%02d:%02d" % (result.starting_date_time.hour,result.starting_date_time.minute,result.starting_date_time.second)
            name = "%s%d (start: %s)" % (ROADMAP_LAYER_NAME,k, start)
            rselect = ResultSelection()
            for k,v in result.costs.iteritems():
                rselect.addCost(Tempus.CostName[k], "%.1f%s" % (v, Tempus.CostUnit[k]))
                if k == Tempus.Cost.Duration:
                    s = result.starting_date_time.hour * 60 + result.starting_date_time.minute + result.starting_date_time.second / 60.0;
                    s += v
                    name = "%s%d (%s -> %s)" % (ROADMAP_LAYER_NAME,k, start, min2hm(s) )
            rselect.setText( name )
            self.dlg.ui.resultSelectionLayout.addWidget( rselect )
            self.result_ids.append( rselect.id() )
            k += 1

        # delete pre existing roadmap layers
        maps = QgsMapLayerRegistry.instance().mapLayers()
        trace_layer = None
        for k,v in maps.items():
            if v.name().startswith(ROADMAP_LAYER_NAME):
                QgsMapLayerRegistry.instance().removeMapLayers( [k] )
            elif v.name().startswith(TRACE_LAYER_NAME):
                trace_layer = v
                d = v.dataProvider()
                ids = [f.id() for f in d.getFeatures()]
                d.deleteFeatures(ids)
                attr_ids = range(len(d.fields()))
                d.deleteAttributes(attr_ids)

        # then display each layer
        k = 1
        for result in results:
            self.displayRoadmapLayer( result.steps, k )
            if result.trace is not None:
                self.displayTrace( result.trace, k, trace_layer )
            k += 1


    def onResultSelected( self, id ):
        for i in range(0, len(self.result_ids)):
            if id == self.result_ids[i]:
                self.displayRoadmapTab( self.results[i] )
                self.selectRoadmapLayer( i+1 )
                self.currentRoadmap = i
                self.dlg.ui.showElevationsBtn.show()
                break


    def onReset( self ):
        if os.path.exists( PREFS_FILE ):
            f = open( PREFS_FILE, 'r' )
            prefs = pickle.load( f )
            self.dlg.loadState( prefs['query'] )
            self.plugin_options = prefs['plugin_options']
            self.update_plugin_options(0)

    #
    # When the 'compute' button gets clicked
    #
    def onCompute(self):
        if self.wps is None:
            return

        # retrieve request data from dialogs
        coords = self.dlg.get_coordinates()
        [ox, oy] = coords[0]
        criteria = self.dlg.get_selected_criteria()
        constraints = self.dlg.get_constraints()
        has_constraint = False
        for i in range(len(constraints)-1):
            ci, ti = constraints[i]
            cj, tj = constraints[i+1]
            dti = datetime.strptime(ti, "%Y-%m-%dT%H:%M:%S")
            dtj = datetime.strptime(tj, "%Y-%m-%dT%H:%M:%S")
            if ci != 0 or cj != 0:
                has_constraint = True
            if ci == 2 and cj == 1 and dtj < dti:
                QMessageBox.warning( self.dlg, "Warning", "Impossible constraint : " + tj + " < " + ti )
                return

        parking = self.dlg.get_parking()
        pvads = self.dlg.get_pvads()
        #networks = [ self.networks[x].id for x in self.dlg.selected_networks() ]
        transports = [ self.transport_modes[x].id for x in self.dlg.selected_transports() ]
        has_pt = False
        for x in self.dlg.selected_transports():
            if self.transport_modes[x].is_public_transport:
                has_pt = True
                break

        if transports == []:
            QMessageBox.warning( self.dlg, "Warning", "No transport mode is selected, defaulting to pedestrian walk")
            transports = [ 1 ]

        if has_pt and not has_constraint:
            QMessageBox.critical( self.dlg, "Inconsistency", "Some public transports are selected, but not time constraint is specified" )
            return

        # build the request
        currentPlugin = str(self.dlg.ui.pluginCombo.currentText())

        steps = []
        for i in range(1, len(constraints)):
            steps.append( Tempus.RequestStep( private_vehicule_at_destination = pvads[i-1],
                                destination = Tempus.Point( coords[i][0], coords[i][1] ),
                                constraint = Tempus.Constraint( type = constraints[i][0], date_time = constraints[i][1] )
                                )
                          )

        try:
            select_xml = self.wps.request( plugin_name = currentPlugin,
                                           plugin_options = self.plugin_options[currentPlugin],
                                           origin = Tempus.Point(ox, oy),
                                           departure_constraint = Tempus.Constraint( type = constraints[0][0],
                                                                                     date_time = constraints[0][1] ),
                                           allowed_transport_modes = transports,
                                           parking_location = None if parking == [] else Tempus.Point(parking[0], parking[1]),
                                           criteria = criteria,
                                           steps = steps
                                           )
        except RuntimeError as e:
            displayError( e.args[1] )
            return

        self.results = self.wps.results
        self.displayResults( self.results )
        self.displayMetrics( self.wps.metrics )

        # enable 'metrics' and 'roadmap' tabs
        self.dlg.ui.verticalTabWidget.setTabEnabled( 3, True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 4, True )
        # clear the roadmap table
        self.dlg.ui.roadmapTable.clear()
        self.dlg.ui.roadmapTable.setRowCount(0)

        # save query / plugin options
        prefs = {}
        prefs['query'] = self.dlg.saveState()
        prefs['plugin_options'] = dict(self.plugin_options)
        f = open( PREFS_FILE, 'w+' )
        pickle.dump( prefs, f )

        # save the request and the server state
        xml_record = '<record>' + select_xml
        server_state = to_xml([ 'server_state',
                                etree.tostring(self.wps.save['plugins']),
                                etree.tostring(self.wps.save['transport_modes']),
                                etree.tostring(self.wps.save['transport_networks']) ] )
        xml_record += server_state + '</record>'
        self.historyFile.addRecord( xml_record )

    def onHistoryItemSelect( self, item ):
        msgBox = QMessageBox()
        msgBox.setIcon( QMessageBox.Warning )
        msgBox.setText( "Warning" )
        msgBox.setInformativeText( "Do you want to load this item from the history ? Current options and query parameters will be overwritten" )
        msgBox.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        ret = msgBox.exec_()
        if ret == QMessageBox.No:
            return

        id = item.data( Qt.UserRole )
        # load from db
        (id, date, xmlStr) = self.historyFile.getRecord( id )

        if XML_SCHEMA_VALIDATION:
                # validate entry
                schema_root = ET.parse( config.TEMPUS_DATA_DIR + "/wps_schemas/record.xsd" )
                schema = ET.XMLSchema(schema_root)
                parser = ET.XMLParser(schema = schema)
                try:
                        tree = ET.fromstring( xmlStr, parser)
                except ET.XMLSyntaxError as e:
                        QMessageBox.warning( self.dlg, "XML parsing error", e.msg )
                        return

        else:
                tree = etree.XML(xmlStr)
        loaded = {}
        for child in tree:
            loaded[ child.tag ] = child

        # reset
        self.clear()

        # update UI
        self.plugins.clear()
        plugins = Tempus.parse_plugins(loaded['server_state'][0])
        for plugin in plugins:
            self.plugins[plugin.name] = plugin
        self.initPluginOptions()
        self.displayPlugins( self.plugins )

        # get current plugin option values
        currentPlugin = loaded['select'][0].attrib['name']
        myoptions = Tempus.parse_plugin_options( loaded['select'][2] )
        for k, v in myoptions.iteritems():
            self.plugin_options[currentPlugin][k] = v.value
        # select currendt plugin
        idx = self.dlg.ui.pluginCombo.findText( currentPlugin )
        self.dlg.ui.pluginCombo.setCurrentIndex( idx )

        self.transport_modes = Tempus.parse_transport_modes( loaded['server_state'][1] )
        self.transport_modes_dict = {}
        for t in self.transport_modes:
                self.transport_modes_dict[t.id] = t
        self.networks = Tempus.parse_transport_networks( loaded['server_state'][2] )

        self.displayTransportAndNetworks()

        self.dlg.loadFromXML( loaded['select'][1] )
        self.displayMetrics( Tempus.parse_metrics(loaded['select'][4]) )
        self.results = Tempus.parse_results(loaded['select'][3])
        self.displayResults( self.results )

        # enable tabs
        self.dlg.ui.verticalTabWidget.setTabEnabled( 1, True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 2, True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 3, True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 4, True )

        # update pinpoints layers
        self.dlg.updateLayers()
        
        # simulate a disconnection
        self.wps = None
        self.dlg.ui.computeBtn.setEnabled(False)
        self.setStateText("DISCONNECTED")

    def onImportHistory( self ):
        fname = QFileDialog.getOpenFileName( None, 'Import history file', '')
        if not fname:
            return

        # switch historyFile
        self.historyFile = ZipHistoryFile( fname )
        self.dlg.ui.historyFileLbl.setText( fname )
        self.loadHistory()

    # when the user selects a row of the roadmap
    def onRoadmapSelectionChanged( self ):
        if self.selectionInProgress:
            return

        selected = self.dlg.ui.roadmapTable.selectedIndexes()
        rows = []
        for s in selected:
            r = s.row()
            if r not in rows: rows.append( r )

        roadmap = self.results[ self.currentRoadmap ]

        # find layer
        layer = None
        lname = "%s%d" % (ROADMAP_LAYER_NAME, self.currentRoadmap)
        maps = QgsMapLayerRegistry.instance().mapLayers()
        for k,v in maps.items():
            if v.name()[0:len(ROADMAP_LAYER_NAME)] == ROADMAP_LAYER_NAME:
                layer = v
                break
        # the layer may have been deleted
        if not layer:
            return

        self.selectionInProgress = True
        layer.selectAll()
        layer.invertSelection() # no deselectAll ??
        for row in rows:
            # row numbers start at 0 and ID starts at 1
            layer.select( row + 1)

        # highlight selection on the elevation profile
        self.profile.highlightSelection( rows )
        self.selectionInProgress = False

    # when selection on the itinerary layer changes
    def onLayerSelectionChanged( self, layer ):
        if self.selectionInProgress:
            return

        n = len(ROADMAP_LAYER_NAME)
        if len(layer.name()) <= n:
            return
        layerid = int(layer.name()[n:])-1
        self.currentRoadmap = layerid

        # in case the layer still exists, but not the underlying result
        if len(self.results) <= layerid:
            return

        # block signals to prevent infinite loop
        self.selectionInProgress = True
        self.displayRoadmapTab( self.results[layerid-1] )

        selected = [ feature.id()-1 for feature in layer.selectedFeatures() ]
        k = 0
        if len(selected) > 0:
                for fid in selected:
                        self.dlg.ui.roadmapTable.selectRow( fid )
                        if k == 0:
                                selectionModel = self.dlg.ui.roadmapTable.selectionModel()
                                selectionItem = selectionModel.selection()
                                k += 1
                        else:
                                selectionItem.merge( selectionModel.selection(), QItemSelectionModel.Select )
                selectionModel.select( selectionItem, QItemSelectionModel.Select )

        self.profile.highlightSelection( selected )
        self.selectionInProgress = False

    def onShowElevations( self ):
        if not self.profile:
            return
        if self.profile.isVisible():
            self.profile.hide()
            self.dlg.ui.showElevationsBtn.setText("Show elevations")
        else:
            self.profile.show()
            self.dlg.ui.showElevationsBtn.setText("Hide elevations")

    def unload(self):
        # Remove the plugin menu item and icon
        self.iface.removePluginMenu(u"&Tempus",self.action)
        self.iface.removePluginMenu(u"&Tempus",self.clipAction)
        self.iface.removePluginMenu(u"&Tempus",self.clipActionFromSel)
        self.iface.removePluginMenu(u"&Tempus",self.loadLayersAction)
        self.iface.removePluginMenu(u"&Tempus",self.exportTraceAction)
        self.iface.removeToolBarIcon(self.action)

    # run method that performs all the real work
    def run(self):
        # show the dialog
        self.dlg.show()
