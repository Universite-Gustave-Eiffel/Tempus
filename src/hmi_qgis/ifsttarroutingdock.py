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
 IfsttarRoutingDialog
                                 A QGIS plugin
 Get routing informations with various algorithms
                             -------------------
        begin                : 2012-04-12
        copyright            : (C) 2012 by Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/
"""

from criterionchooser import CriterionChooser
from stepselector import StepSelector

import os
import config

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from ui_ifsttarrouting import Ui_IfsttarRoutingDock

from wps_client import *

from qgis.core import *

def display_pinpoints( coords, start_letter, style_file, layer_name, canvas ):
    maps = QgsMapLayerRegistry.instance().mapLayers()
    vl = None
    features = {}
    for k,v in maps.items():
        if v.name()[0:len(layer_name)] == layer_name:
            vl = v
            # remove all features
            iter = vl.getFeatures()
            ids = [ f.id() for f in iter ]
            vl.dataProvider().deleteFeatures( ids )
            break

    if vl is None:
        vl = QgsVectorLayer("Point?crs=epsg:2154", layer_name, "memory")
        pr = vl.dataProvider()
        vl.startEditing()
        vl.addAttribute( QgsField( "tag", QVariant.String ) )
        vl.commitChanges()
        vl.updateExtents()
        vl.loadNamedStyle( config.DATA_DIR + "/" + style_file )
        QgsMapLayerRegistry.instance().addMapLayers( [vl] )

    if coords is not None:
        pr = vl.dataProvider()
        i = 0
        for coord in coords:
            fet = QgsFeature()
            pt = QgsPoint( coord[0], coord[1] )
            geo = QgsGeometry.fromPoint( pt )
            fet.setGeometry( geo )
            tag = chr(ord( start_letter )+i)
            fet.setAttributes( [tag] )
            pr.addFeatures( [fet] )
            i = i + 1

    # refresh canvas
    canvas.refresh()

# create the dialog for route queries
class IfsttarRoutingDock(QDockWidget):
    #
    # Set widgets' states from a hash
    #
    def loadState( self, state ):
        self.ui.wpsUrlText.setText( state['wps_url_text'] )
        self.ui.pluginCombo.setCurrentIndex( state['plugin_selected'] )
        
        self.set_steps( state['nsteps'] )
        self.set_selected_criteria( state['criteria'] )
        self.set_coordinates( state['coordinates'] )
        self.set_constraints( state['constraints'] )
        self.set_parking( state['parking'] )
        self.set_pvads( state['pvads'] )
        self.set_selected_transports( state['transports'] )

    def loadFromXML( self, xml ):
        pson = to_pson(xml)

        criteria = []
        steps = []
        networks = []
        modes = []
        parking = None
        origin = None
        dep = None
        for child in pson[1:]:
            if child[0] == 'origin':
                origin = child
            elif child[0] == 'parking_location':
                parking = child
            elif child[0] == 'optimizing_criterion':
                criteria.append(int(child[1]))
            elif child[0] == 'allowed_network':
                networks.append(int(child[1]))
            elif child[0] == 'allowed_mode':
                modes.append(int(child[1]))
            elif child[0] == 'step':
                steps.append(child)

        def readCoords( n ):
            if n[1].has_key('vertex'):
                # if the point is given by ID
                return None
            return [ float(n[1]['x']), float(n[1]['y']) ]

        if parking:
            parking = readCoords(parking)
            self.set_parking( parking )
        self.set_steps( len(steps) )

        coords = [ readCoords(origin) ]
        constraints = []
        pvads = []
        for step in steps:
            for p in step[2:]:
                if p[0] == 'destination':
                    coords.append( readCoords(p) )
                elif p[0] == 'constraint':
                    c = p[1]
                    constraints.append( [int(c['type']), c['date_time'] ] )
            pvads.append( step[1]['private_vehicule_at_destination'] == 'true' )

        self.set_selected_criteria( criteria )
        self.set_coordinates( coords )
        self.set_pvads( pvads )
        self.set_constraints( constraints )

        self.set_selected_transports_from_id( modes )
        
    #
    # Save widgets' states
    #
    def saveState( self ):
        state = {
            'wps_url_text': self.ui.wpsUrlText.text(),
            'plugin_selected': self.ui.pluginCombo.currentIndex(),
            'nsteps': self.nsteps(),
            'criteria' : self.get_selected_criteria(),
            'coordinates': self.get_coordinates(),
            'constraints': self.get_constraints(),
            'parking' : self.get_parking(),
            'pvads': self.get_pvads(),
            'transports': self.selected_transports()
            }
        return state

    def __init__(self, canvas):
        QDockWidget.__init__(self)
        self.canvas = canvas
        self.in_query = False

        # Set up the user interface from Designer.
        self.ui = Ui_IfsttarRoutingDock()
        self.ui.setupUi(self)

        self.ui.criterionBox.addWidget( CriterionChooser( self.ui.criterionBox, True ) )

        # set roadmap's header
        self.ui.roadmapTable.setHorizontalHeader(QHeaderView(Qt.Horizontal))
        self.ui.roadmapTable.setHorizontalHeaderLabels( ["", "Direction", "Costs"] )

        # add the origin chooser
        self.ui.origin = StepSelector( self.ui.verticalLayout, "Origin",
                                       dock = self )
        self.ui.verticalLayout.insertWidget( 0, self.ui.origin )
        self.ui.origin.set_canvas( self.canvas )

        # add the Destination chooser
        dest = StepSelector( self.ui.stepBox, "Destination",
                             coordinates_only = False,
                             dock = self )
        dest.coordinates_changed.connect( self.update_pinpoints )
        dest.set_canvas( self.canvas )
        self.ui.stepBox.addWidget( dest )

        # set the minimum height of the scroll area to "one stepselector"
        self.ui.scrollArea.setMinimumHeight( dest.sizeHint().height() )

        # set pin points updater
        self.ui.origin.coordinates_changed.connect( self.update_pinpoints )
        self.ui.origin.dock = self

        # add the private parking chooser
        self.parkingChooser = StepSelector( self.ui.queryPage, "Private parking location",
                                            coordinates_only = True,
                                            dock = self )
        self.parkingChooser.set_canvas( self.canvas )
        self.parkingEnabledBox = QCheckBox( self.ui.queryPage )
        self.parkingEnabledBox.stateChanged.connect( self.on_toggle_parking )
        self.parkingEnabledBox.setCheckState( Qt.Unchecked )
        self.ui.parkingLayout.addWidget( self.parkingEnabledBox )
        self.ui.parkingLayout.addWidget( self.parkingChooser )

        # set parking location updater
        self.parkingChooser.coordinates_changed.connect( self.update_parking )
        self.parkingChooser.dock = self

    def on_toggle_parking( self, state ):
        self.parkingChooser.setEnabled( state == Qt.Checked )
        self.update_parking()

    def updateLayers( self ):
        "Update pinpoints and parking layers"
        self.update_pinpoints()
        self.update_parking()

    def get_selected_criteria( self ):
        s = []
        for c in range( 0, self.ui.criterionBox.count() ):
            s.append( self.ui.criterionBox.itemAt( c ).widget().selected() )
        return s

    def set_selected_criteria( self, sel ):
        c = self.ui.criterionBox.count()
        for i in range(0, c-1):
            self.ui.criterionBox.itemAt(1).widget().onRemove()
        for i in range(0, len(sel)-1 ):
            self.ui.criterionBox.itemAt(0).widget().onAdd()

        i = 0
        for selected in sel:
            self.ui.criterionBox.itemAt(i).widget().set_selection( selected )
            i += 1

    def set_supported_criteria( self, criteria ):
        c = self.ui.criterionBox.count()
        for i in range(c):
            self.ui.criterionBox.itemAt(i).widget().set_supported_criteria( criteria )

    def set_intermediate_steps_support( self, enabled ):
        n = self.nsteps()
        for i in range(0, n):
            w = self.ui.stepBox.itemAt(i).widget()
            w.plusBtn.setEnabled( enabled )

    def set_depart_after_support( self, enabled ):
        self.ui.origin.set_depart_after_support(enabled)
        n = self.nsteps()
        for i in range(0, n):
            w = self.ui.stepBox.itemAt(i).widget()
            w.set_depart_after_support(enabled)

    def set_arrive_before_support( self, enabled ):
        self.ui.origin.set_arrive_before_support(enabled)
        n = self.nsteps()
        for i in range(0, n):
            w = self.ui.stepBox.itemAt(i).widget()
            w.set_arrive_before_support(enabled)

    # get coordinates of all steps
    def get_coordinates( self ):
        coords = [ self.ui.origin.get_coordinates() ]
        n = self.nsteps()
        for i in range(0, n):
            w = self.ui.stepBox.itemAt(i).widget()
            coord = w.get_coordinates()
            coords.append( coord )
        return coords

    def nsteps( self ):
        return self.ui.stepBox.count()

    def set_coordinates( self, coords ):
        self.ui.origin.set_coordinates( coords[0] )
        n = 0
        for coord in coords[1:]:
            if self.ui.stepBox.count() > n:
                self.ui.stepBox.itemAt( n ).widget().set_coordinates( coord )
            n += 1

    def get_constraints( self ):
        c = []
        n = self.nsteps()
        for i in range(0, n):
            t = self.ui.stepBox.itemAt( i ).widget().get_constraint_type()
            cs = self.ui.stepBox.itemAt( i ).widget().get_constraint()
            c.append( [ t, cs ] )
        return c

    def set_constraints( self, constraints ):
        i = 0
        for constraint in constraints[1:]:
            self.ui.stepBox.itemAt( i ).widget().set_constraint_type( constraint[0] )
            self.ui.stepBox.itemAt( i ).widget().set_constraint( constraint[1] )
            i += 1

    def get_pvads( self ):
        pvads = []
        n = self.nsteps()
        for i in range(0, n):
            pvads.append( self.ui.stepBox.itemAt( i ).widget().get_pvad() )
        return pvads

    def set_pvads( self, pvads ):
        i = 0
        for pvad in pvads:
            self.ui.stepBox.itemAt( i ).widget().set_pvad( pvad )
            i += 1

    def selected_networks( self ):
        s = []
        model = self.ui.networkList.model()
        n = model.rowCount()
        for i in range(0, n):
            v = model.data( model.index(i,0), Qt.CheckStateRole )
            if v == Qt.Checked:
                s.append( i )
        return s

    def selected_transports( self ):
        s = []
        model = self.ui.transportList.model()
        if model is None:
            return s
        n = model.rowCount()
        for i in range(0, n):
            v = model.data( model.index(i,0), Qt.CheckStateRole )
            if v == Qt.Checked:
                s.append( i )
        return s

    def set_selected_networks( self, sel ):
        model = self.ui.networkList.model()
        n = model.rowCount()
        for i in range(0, n):
            if i in sel:
                q = Qt.Checked
            else:
                q = Qt.Unchecked
            model.setData( model.index(i,0), q, Qt.CheckStateRole )

    def set_selected_transports( self, sel ):
        model = self.ui.transportList.model()
        if model is None:
            return
        n = model.rowCount()
        for i in range(0, n):
            if i in sel:
                q = Qt.Checked
            else:
                q = Qt.Unchecked
            model.setData( model.index(i,0), q, Qt.CheckStateRole )

    def set_selected_transports_from_id( self, sel ):
        model = self.ui.transportList.model()
        n = model.rowCount()
        for i in range(0, n):
            id = model.data( model.index(i,0), Qt.UserRole )
            if id in sel:
                q = Qt.Checked
            else:
                q = Qt.Unchecked
            model.setData( model.index(i,0), q, Qt.CheckStateRole )

    def get_parking( self ):
        if self.parkingEnabledBox.checkState() == Qt.Checked:
            [x,y] = self.parkingChooser.get_coordinates()
            if x < 0.001:
                return []
            return [x,y]
        return []

    def set_parking( self, xy ):
        self.parkingEnabledBox.setCheckState( Qt.Unchecked if xy == [] else Qt.Checked )
        if xy != []:
            self.parkingChooser.set_coordinates( xy )

    def set_steps( self, nsteps ):
        # first remove all intermediary steps
        c = self.ui.stepBox.count()
        # remove from idx 1 to N-1 (skip destination)
        for i in range(1, c):
            self.ui.stepBox.itemAt(0).widget().onRemove()
        
        for i in range(0, nsteps-1):
            self.ui.stepBox.itemAt(0).widget().onAdd()

    def reset( self ):
        self.newQuery = True

    def inQuery( self ):
        self.in_query = True
        self.updateLayers()

    def resetCoordinates( self ):
        "called by StepSelector on coordinate modification"
        # always delete the roadmap
        l = 'Tempus_Roadmap_'
        maps = QgsMapLayerRegistry.instance().mapLayers()
        for k,v in maps.items():
            if v.name()[0:len(l)] == l:
                QgsMapLayerRegistry.instance().removeMapLayers( [v.id()] )
                break

    def update_pinpoints( self ):
        if self.in_query:
            display_pinpoints( self.get_coordinates(), 'A', 'style_pinpoints.qml', 'Tempus_pin_points', self.canvas )

    def update_parking( self ):
        c = self.get_parking()
        if c == []:
            p = None
        else:
            p = [c]
        display_pinpoints( p, 'P', 'style_parking.qml', 'Tempus_private_parking', self.canvas )

