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
from PyQt4 import QtCore, QtGui
from qgis.core import *
from qgis.gui import *
import re
from wps_client import *
import config
import binascii

# Initialize Qt resources from file resources.py
import resources_rc
# Import the code for the dialog
from ifsttarroutingdock import IfsttarRoutingDock
# Import the splash screen
from ui_splash_screen import Ui_SplashScreen

from result_selection import ResultSelection

def format_cost( cost ):
    cost_id = int(cost.attrib['type'])
    cost_value = float(cost.attrib['value'])
    cost_name = ''
    cost_unit = ''
    if cost_id == 1:
        cost_name = 'Distance'
        cost_unit = 'm'
    elif cost_id == 2:
        cost_name = 'Duration'
        cost_unit = 's'
    elif cost_id == 3:
        cost_name = 'Price'
        cost_unit = '€'
    elif cost_id == 4:
        cost_name = 'Carbon'
        cost_unit = '?'
    elif cost_id == 5:
        cost_name = 'Calories'
        cost_unit = ''
    elif cost_id == 6:
        cost_name = 'Number of changes'
        cost_unit = ''
    elif cost_id == 7:
        cost_name = 'Variability'
        cost_unit = ''
    return "%s: %.1f %s" % (cost_name, cost_value, cost_unit)

class SplashScreen(QtGui.QDialog):
    def __init__(self):
        QtGui.QDialog.__init__(self)
        self.ui = Ui_SplashScreen()
        self.ui.setupUi(self)

class IfsttarRouting:

    def __init__(self, iface):
        # Save reference to the QGIS interface
        self.iface = iface
        # Create the dialog and keep reference
        self.canvas = self.iface.mapCanvas()
        self.dlg = IfsttarRoutingDock( self.canvas )
        # show the splash screen
        self.splash = SplashScreen()

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

        self.wps = None

    def initGui(self):
        # Create action that will start plugin configuration
        self.action = QAction(QIcon(":/plugins/ifsttarrouting/icon.png"), \
            u"Compute route", self.iface.mainWindow())
        # connect the action to the run method
        QObject.connect(self.action, SIGNAL("triggered()"), self.run)

        QObject.connect(self.dlg.ui.connectBtn, SIGNAL("clicked()"), self.onConnect)
        QObject.connect(self.dlg.ui.computeBtn, SIGNAL("clicked()"), self.onCompute)
        QObject.connect(self.dlg.ui.verticalTabWidget, SIGNAL("currentChanged( int )"), self.onTabChanged)
        QObject.connect(self.dlg.ui.buildBtn, SIGNAL("clicked()"), self.onBuildGraphs)

        QObject.connect( self.dlg.ui.pluginCombo, SIGNAL("currentIndexChanged(int)"), self.update_plugin_options )

        self.originPoint = QgsPoint()
        self.destinationPoint = QgsPoint()

        # Add toolbar button and menu item
        self.iface.addToolBarIcon(self.action)
        self.iface.addPluginToMenu(u"&Compute route", self.action)
        self.iface.addDockWidget( Qt.LeftDockWidgetArea, self.dlg )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 1, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 2, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 3, False )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 4, False )

    # Get current WPS server state
    def updateState( self ):
        self.state = -1
        try:
            outputs = self.wps.execute( 'state', {} )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return

        for name, content in outputs.items():
            if name == 'db_options':
                if content.text is not None:
                    self.dlg.ui.dbOptionsText.setText( content.text )
            elif name == 'state':
                self.state = int( content.text )
                if self.state == 0:
                    state_text = 'Started'
                elif self.state == 1:
                    state_text = 'Connected'
                elif self.state == 2:
                    state_text = 'Graph pre-built'
                elif self.state == 3:
                    state_text = 'Graph built'

                self.dlg.ui.stateText.setText( state_text )
        # enable query tab only if the database is loaded
        if self.state >= 1:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 1, True )
        else:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 1, False )

        if self.state >= 3:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 2, True )
        else:
            self.dlg.ui.verticalTabWidget.setTabEnabled( 2, False )

    # when the 'connect' button get clicked
    def onConnect( self ):
        # get the wps url
        g = re.search( 'http://([^/]+)(.*)', str(self.dlg.ui.wpsUrlText.text()) )
        host = g.group(1)
        path = g.group(2)
        connection = HttpCgiConnection( host, path )
        self.wps = WPSClient(connection)

        self.updateState()

        # get plugin list
        try:
            outputs = self.wps.execute( 'plugin_list', {} )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return

        self.dlg.ui.pluginCombo.clear()
        plugins = outputs['plugins']
        for plugin in plugins:
            self.dlg.ui.pluginCombo.insertItem(0, plugin.attrib['name'] )
            
        # enable db_options text edit
        self.dlg.ui.dbOptionsText.setEnabled( True )
        self.dlg.ui.dbOptionsLbl.setEnabled( True )
        self.dlg.ui.pluginCombo.setEnabled( True )
        self.dlg.ui.stateLbl.setEnabled( True )
        self.dlg.ui.buildBtn.setEnabled( True )

    def clearLayout( self, lay ):
        # clean the widget list
        rc = lay.rowCount()
        if rc > 0:
            for row in range(0, rc):
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

    def update_plugin_options( self, plugin_idx ):
        if self.dlg.ui.verticalTabWidget.currentIndex() != 1:
            return

        plugin_arg = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ] }

        try:
            outputs = self.wps.execute( 'get_option_descriptions', plugin_arg )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return
        options = outputs['options']
        
        try:
            outputs = self.wps.execute( 'get_options', plugin_arg )
        except RuntimeError as e:
            QMessageBox.warning( None, "Error", e.args[1] )
            return
        options_value = outputs['options']
        
        option_value = {}
        for option_val in options_value:
            option_value[ option_val.attrib['name'] ] = option_val.attrib['value']

        lay = self.dlg.ui.optionsLayout
        self.clearLayout( lay )

        row = 0
        for option in options:
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

    def onTabChanged( self, tab ):
        # 'Query' tab
        if tab == 2:
            if self.wps is None:
                return
            try:
                outputs = self.wps.execute( 'constant_list', {} )
            except RuntimeError as e:
                QMessageBox.warning( self.dlg, "Error", e.args[1] )
                return

            #
            # add each transport type to the list
            #

            # retrieve the list of transport types
            self.transport_type = []
            for transport_type in outputs['transport_types']:
                self.transport_type.append(transport_type.attrib)

            # retrieve the network list
            self.network = []
            for network in outputs['transport_networks']:
                self.network.append(network.attrib)

            listModel = QStandardItemModel()
            for ttype in self.transport_type:
                need_network = int(ttype['need_network'])
                idt = int(ttype['id'])
                has_network = False
                if need_network:
                    # look for networks that provide this kind of transport
                    for network in self.network:
                        ptt = int(network['provided_transport_types'])
                        if ptt & idt:
                            has_network = True
                            break

                item = QStandardItem( ttype['name'] )
                if need_network and not has_network:
                    item.setFlags(Qt.ItemIsUserCheckable)
                    item.setData(QVariant(Qt.Unchecked), Qt.CheckStateRole)
                else:
                    item.setFlags(Qt.ItemIsUserCheckable | Qt.ItemIsEnabled)
                    item.setData(QVariant(Qt.Checked), Qt.CheckStateRole)
                listModel.appendRow(item)
            self.dlg.ui.transportList.setModel( listModel )
                
            networkListModel = QStandardItemModel()
            for network in self.network:
                title = network['name']

                # look for provided transport types
                tlist = []
                ptt = int( network['provided_transport_types'] )
                for ttype in self.transport_type:
                    if ptt & int(ttype['id']):
                        tlist.append( ttype['name'] )

                item = QStandardItem( network['name'] + ' (' + ', '.join(tlist) + ')')
                item.setFlags(Qt.ItemIsUserCheckable | Qt.ItemIsEnabled)
                item.setData(QVariant(Qt.Checked), Qt.CheckStateRole)
                networkListModel.appendRow(item)
            self.dlg.ui.networkList.setModel( networkListModel )

    def onOptionChanged( self, option_name, option_type, val ):
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

        try:
            outputs = self.wps.execute( 'set_options', args )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )

    # when the 'build' button get clicked
    def onBuildGraphs(self):
        self.dlg.ui.buildBtn.setEnabled( False )
        # get the db options
        db_options = str(self.dlg.ui.dbOptionsText.text())

        try:
            self.wps.execute( 'connect', { 'db_options' : [True, ['db_options', db_options ]] } )
            self.wps.execute( 'pre_build', {} )
            self.wps.execute( 'build', {} )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return

        self.updateState()
        self.dlg.ui.buildBtn.setEnabled( True )
        
    def onCompute(self):
        coords = self.dlg.get_coordinates()
        [ox, oy] = coords[0]
        
        criteria = self.dlg.selected_criteria()

        constraints = self.dlg.get_constraints()

        parking = self.dlg.get_parking()

        pvads = self.dlg.get_pvads()

        networks = [ self.network[x] for x in self.dlg.selected_networks() ]
        transports = [ self.transport_type[x] for x in self.dlg.selected_transports() ]

        r = [ 'request',
            ['origin', ['x', str(ox)], ['y', str(oy)] ],
            ['departure_constraint', { 'type': constraints[0][0], 'date_time': constraints[0][1] } ]]

        if parking != []:
            r.append(['parking_location', ['x', parking[0]], ['y', parking[1]] ])

        for criterion in criteria:
            r.append(['optimizing_criterion', criterion])

        allowed_transports = 0
        for x in transports:
            allowed_transports += int(x['id'])

        r.append( ['allowed_transport_types', allowed_transports ] )

        for n in networks:
            r.append( ['allowed_network', int(n['id']) ] )

        n = len(constraints)
        for i in range(1, n):
            pvad = 'false'
            if pvads[i] == True:
                pvad = 'true'
            r.append( ['step',
                       [ 'destination', ['x', str(coords[i][0])], ['y', str(coords[i][1])] ],
                       [ 'constraint', { 'type' : constraints[i][0], 'date_time': constraints[i][1] } ],
                       [ 'private_vehicule_at_destination', pvad ]
                       ] )

        plugin_arg = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ] }

        args = plugin_arg
        args['request'] = [ True, r ]
        try:
            self.wps.execute( 'pre_process', args )
            self.wps.execute( 'process', plugin_arg )
            outputs = self.wps.execute( 'result', plugin_arg )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return

        #
        # create layer

        # first, we remove the first layer found with the same name to reuse it
        lname = "Itinerary"
        maps = QgsMapLayerRegistry.instance().mapLayers()
        for k,v in maps.items():
            if v.name() == lname:
                vl = v
                QgsMapLayerRegistry.instance().removeMapLayers( [k] )
                break

        # create a new vector layer
        vl = QgsVectorLayer("LineString?crs=epsg:2154", lname, "memory")

        # road steps are red
        # public transport steps are blue
        # others are yellow
        root_rule = QgsRuleBasedRendererV2.Rule( QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : '255,255,0'} ))
        rule1 = QgsRuleBasedRendererV2.Rule( QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : '255,0,0'}),
                                             0,
                                             0,
                                             "transport_type=1" )
        rule2 = QgsRuleBasedRendererV2.Rule( QgsLineSymbolV2.createSimple({'width': '1.5', 'color' : '0,0,255'}),
                                             0,
                                             0,
                                             "transport_type=2" )
        root_rule.appendChild( rule1 )
        root_rule.appendChild( rule2 )
        renderer = QgsRuleBasedRendererV2( root_rule );
        vl.setRendererV2( renderer )

        pr = vl.dataProvider()

        pr.addAttributes( [ QgsField( "transport_type", QVariant.Int ) ] )
        
        # browse steps
        steps = outputs['result']
        for step in steps:
            # find wkb geometry
            wkb=''
            for prop in step:
                if prop.tag == 'wkb':
                    wkb = prop.text

            fet = QgsFeature()
            geo = QgsGeometry()
            geo.fromWkb( binascii.unhexlify(wkb) )
            if step.tag == 'road_step':
                transport_type = 1
            elif step.tag == 'public_transport_step':
                transport_type = 2
            else:
                transport_type = 3
            fet.setAttributeMap( { 0: QVariant( transport_type ) } )
            fet.setGeometry( geo )
            pr.addFeatures( [fet] )

        # We MUST call this manually (see http://hub.qgis.org/issues/4687)
        vl.updateFieldMap()
        # update layer's extent when new features have been added
        # because change of extent in provider is not propagated to the layer
        vl.updateExtents()

        QgsMapLayerRegistry.instance().addMapLayer(vl)

        # get the roadmap
#        rselect = ResultSelection()
#        rselect.addCost("Distance", "167km")
#        self.dlg.ui.resultSelectionLayout.addWidget( rselect )

        last_movement = 0
        roadmap = outputs['result']
        row = 0
        self.dlg.ui.roadmapTable.clear()
        self.dlg.ui.roadmapTable.setRowCount(0)
        for step in roadmap:
            text = ''
            icon_text = ''
            cost_text = ''
            if step.tag == 'road_step':
                road_name = step[0].text or ''
                movement = int(step[1].text)
                costs = step[2:-1]
                text += "<p>"
                action_txt = 'Walk on '
                if last_movement == 1:
                    icon_text += "<img src=\"%s/turn_left.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                    action_txt = "Turn left on "
                elif last_movement == 2:
                    icon_text += "<img src=\"%s/turn_right.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                    action_txt = "Turn right on "
                elif last_movement >= 4 and last_movement < 999:
                    icon_text += "<img src=\"%s/roundabout.png\" width=\"24\" height=\"24\"/>" % config.DATA_DIR
                text += action_txt + road_name + "<br/>\n"
                for cost in costs:
                    cost_text += format_cost( cost ) + "<br/>\n"
                text += "</p>"
                last_movement = movement

            elif step.tag == 'public_transport_step':
                network = step[0].text
                departure = step[1].text
                arrival = step[2].text
                trip = step[3].text
                costs = step[4:-1]
                for cost in costs:
                    cost_text += format_cost( cost ) + "<br/>\n"
                # set network text as icon
                icon_text = network
                text = "Take the trip %s from '%s' to '%s'" % (trip, departure, arrival)
                
            self.dlg.ui.roadmapTable.insertRow( row )
            descLbl = QLabel()
            descLbl.setText( text )
            descLbl.setMargin( 5 )
            self.dlg.ui.roadmapTable.setCellWidget( row, 1, descLbl )

            if icon_text != '':
                lbl = QLabel()
                lbl.setText( icon_text )
                lbl.setMargin(5)
                self.dlg.ui.roadmapTable.setCellWidget( row, 0, lbl )

            costText = QLabel()
            costText.setText( cost_text )
            costText.setMargin( 5 )
            self.dlg.ui.roadmapTable.setCellWidget( row, 2, costText )
            self.dlg.ui.roadmapTable.resizeRowToContents( row )
            row += 1
        # Adjust column widths
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(0)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 0, w )
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(1)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 1, w )
        w = self.dlg.ui.roadmapTable.sizeHintForColumn(2)
        self.dlg.ui.roadmapTable.horizontalHeader().resizeSection( 2, w )

        # get metrics
        plugin_arg = { 'plugin' : [ True, ['plugin', {'name' : str(self.dlg.ui.pluginCombo.currentText())} ] ] }
        try:
            outputs = self.wps.execute( 'get_metrics', plugin_arg )
        except RuntimeError as e:
            QMessageBox.warning( self.dlg, "Error", e.args[1] )
            return
        metrics = outputs['metrics']

        row = 0
        self.clearLayout( self.dlg.ui.resultLayout )

        for metric in metrics:
            lay = self.dlg.ui.resultLayout
            lbl = QLabel( self.dlg )
            lbl.setText( metric.attrib['name'] + '' )
            lay.setWidget( row, QFormLayout.LabelRole, lbl )
            widget = QLineEdit( self.dlg )
            widget.setText( metric.attrib['value'] + '' )
            widget.setEnabled( False )
            lay.setWidget( row, QFormLayout.FieldRole, widget )
            row += 1

        self.dlg.ui.verticalTabWidget.setTabEnabled( 3, True )
        self.dlg.ui.verticalTabWidget.setTabEnabled( 4, True )

    def unload(self):
        # Remove the plugin menu item and icon
        self.iface.removePluginMenu(u"&Compute route",self.action)
        self.iface.removeToolBarIcon(self.action)

    # run method that performs all the real work
    def run(self):
        self.splash.show()
        # show the dialog
        self.dlg.show()
