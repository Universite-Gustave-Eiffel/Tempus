# -*- coding: utf-8 -*-
"""
/***************************************************************************
 IfsttarRoutingDialog
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

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from ui_ifsttarrouting import Ui_IfsttarRoutingDock

from criterionchooser import CriterionChooser
from stepselector import StepSelector

import os
import pickle

PREFS_FILE = os.path.expanduser('~/.ifsttarrouting.prefs')

# create the dialog for route queries
class IfsttarRoutingDock(QtGui.QDockWidget):
    #
    # Set widgets' states from a hash
    #
    def loadState( self, state ):
        self.ui.wpsUrlText.setText( state['wps_url_text'] )
        self.ui.dbOptionsText.setText( state['db_options'] )
        self.ui.pluginCombo.setCurrentIndex( state['plugin_selected'] )
        
        self.set_steps( state['nsteps'] )
        self.set_selected_criteria( state['criteria'] )
        self.set_coordinates( state['coordinates'] )
        self.set_constraints( state['constraints'] )
        self.set_parking( state['parking'] )
        self.set_pvads( state['pvads'] )

    #
    # Save widgets' states
    #
    def saveState( self ):
        state = {
            'wps_url_text': self.ui.wpsUrlText.text(),
            'plugin_selected': self.ui.pluginCombo.currentIndex(),
            'db_options': self.ui.dbOptionsText.text(),
            'nsteps': self.nsteps(),
            'criteria' : self.get_selected_criteria(),
            'coordinates': self.get_coordinates(),
            'constraints': self.get_constraints(),
            'parking' : self.get_parking(),
            'pvads': self.get_pvads(),
            }
        return state

    def __init__(self, canvas):
        QtGui.QDockWidget.__init__(self)
        self.canvas = canvas
        # Set up the user interface from Designer.
        self.ui = Ui_IfsttarRoutingDock()
        self.ui.setupUi(self)

        self.ui.criterionBox.addWidget( CriterionChooser( self.ui.criterionBox, True ) )

        # set roadmap's header
        self.ui.roadmapTable.setHorizontalHeader(QHeaderView(Qt.Horizontal))

        # add the Destination chooser
        self.ui.origin.set_canvas( self.canvas )
        dest = StepSelector( self.ui.stepBox, "Destination" )
        dest.set_canvas( self.canvas )
        self.ui.stepBox.addWidget( dest )

        # add the private parking chooser
        self.parkingChooser = StepSelector( self.ui.queryPage, "Private parking location", True )
        self.parkingChooser.set_canvas( self.canvas )
        self.ui.parkingLayout.addWidget( self.parkingChooser )

        # preferences is an object used to store user preferences
        if os.path.exists( PREFS_FILE ):
            f = open( PREFS_FILE, 'r' )
            self.prefs = pickle.load( f )
            self.loadState( self.prefs )
        else:
            self.prefs = {}

    def closeEvent( self, event ):
        self.prefs = self.saveState()
        f = open( PREFS_FILE, 'w+' )
        pickle.dump( self.prefs, f )
        event.accept()

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
        c = [ [self.ui.origin.get_constraint_type(), self.ui.origin.get_constraint()] ]
        n = self.nsteps()
        for i in range(0, n):
            t = self.ui.stepBox.itemAt( i ).widget().get_constraint_type()
            cs = self.ui.stepBox.itemAt( i ).widget().get_constraint()
            c.append( [ t, cs ] )
        return c

    def set_constraints( self, constraints ):
        self.ui.origin.set_constraint_type( constraints[0][0] )
        self.ui.origin.set_constraint( constraints[0][1] )
        i = 0
        for constraint in constraints[1:]:
            self.ui.stepBox.itemAt( i ).widget().set_constraint_type( constraint[0] )
            self.ui.stepBox.itemAt( i ).widget().set_constraint( constraint[1] )
            i += 1

    def get_pvads( self ):
        pvads = [ self.ui.origin.get_pvad() ]
        n = self.nsteps()
        for i in range(0, n):
            pvads.append( self.ui.stepBox.itemAt( i ).widget().get_pvad() )
        return pvads

    def set_pvads( self, pvads ):
        self.ui.origin.set_pvad( pvads[0] )
        i = 0
        for pvad in pvads[1:]:
            self.ui.stepBox.itemAt( i ).widget().set_pvad( pvad )
            i += 1

    def selected_networks( self ):
        s = []
        model = self.ui.networkList.model()
        n = model.rowCount()
        for i in range(0, n):
            v = model.data( model.index(i,0), Qt.CheckStateRole )
            if v.toBool():
                s.append( i )
        return s

    def selected_transports( self ):
        s = []
        model = self.ui.transportList.model()
        n = model.rowCount()
        for i in range(0, n):
            v = model.data( model.index(i,0), Qt.CheckStateRole )
            if v.toBool():
                s.append( i )
        return s

    def set_selected_networks( self, sel ):
        model = self.ui.networkList.model()
        n = model.rowCount()
        for i in range(0, n):
            if i in sel:
                q = QVariant(Qt.Checked)
            else:
                q = QVariant(Qt.Unchecked)
            model.setData( model.index(i,0), q, Qt.CheckStateRole )

    def set_selected_transports( self, sel ):
        model = self.ui.transportList.model()
        n = model.rowCount()
        for i in range(0, n):
            if i in sel:
                q = QVariant(Qt.Checked)
            else:
                q = QVariant(Qt.Unchecked)
            model.setData( model.index(i,0), q, Qt.CheckStateRole )

    def get_parking( self ):
        [x,y] = self.parkingChooser.get_coordinates()
        if x < 0.001:
            return []
        return [x,y]

    def set_parking( self, xy ):
        self.parkingChooser.set_coordinates( xy )

    def set_steps( self, nsteps ):
        # first remove all intermediary steps
        c = self.ui.stepBox.count()
        # remove from idx 1 to N-1 (skip destination)
        for i in range(1, c):
            self.ui.stepBox.itemAt(0).widget().onRemove()
        
        for i in range(0, nsteps-1):
            self.ui.stepBox.itemAt(0).widget().onAdd()
