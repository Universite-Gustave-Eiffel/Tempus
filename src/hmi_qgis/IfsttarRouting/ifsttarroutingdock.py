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
from ui_ifsttarrouting import Ui_IfsttarRoutingDock

from criterionchooser import CriterionChooser
from stepselector import StepSelector

import os
import pickle

PREFS_FILE = os.path.expanduser('~/.ifsttarrouting.prefs')

# create the dialog for zoom to point
class IfsttarRoutingDock(QtGui.QDockWidget):
    def __init__(self, canvas):
        QtGui.QDockWidget.__init__(self)
        self.canvas = canvas
        # Set up the user interface from Designer.
        self.ui = Ui_IfsttarRoutingDock()
        self.ui.setupUi(self)

        self.ui.criterionBox.addWidget( CriterionChooser( self.ui.criterionBox, True ) )

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
            self.ui.wpsUrlText.setText( self.prefs['wps_url_text'] )
            self.ui.pluginCombo.setCurrentIndex( self.prefs['plugin_selected'] )
            nsteps = self.prefs['nsteps']
            for i in range(0, nsteps-1):
                self.ui.stepBox.itemAt(0).widget().onAdd()

            self.set_coordinates( self.prefs['coordinates'] )
            self.set_constraints( self.prefs['constraints'] )
            self.set_pvads( self.prefs['pvads'] )
        else:
            self.prefs = {}

    def closeEvent( self, event ):
        self.prefs['wps_url_text'] = self.ui.wpsUrlText.text()
        self.prefs['plugin_selected'] = self.ui.pluginCombo.currentIndex()
        self.prefs['nsteps'] = self.nsteps()
        self.prefs['coordinates'] = self.get_coordinates()
        self.prefs['constraints'] = self.get_constraints()
        self.prefs['pvads'] = self.get_pvads()
        f = open( PREFS_FILE, 'w+' )
        pickle.dump( self.prefs, f )
        event.accept()

    def selected_criteria( self ):
        s = []
        for c in range( 0, self.ui.criterionBox.count() ):
            s.append( self.ui.criterionBox.itemAt( c ).widget().selected() )
        return s

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
        s = self.ui.networkList.selectedIndexes()
        return [ x.row() for x in s ]

    def selected_transports( self ):
        s = self.ui.transportList.selectedIndexes()
        return [ x.row() for x in s ]

    def get_parking( self ):
        [x,y] = self.parkingChooser.get_coordinates()
        if x < 0.001:
            return []
        return [x,y]
