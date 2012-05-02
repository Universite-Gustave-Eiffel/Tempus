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

import os
import pickle

PREFS_FILE = os.path.expanduser('.ifsttarrouting.prefs')

# create the dialog for zoom to point
class IfsttarRoutingDock(QtGui.QDockWidget):
    def __init__(self):
        QtGui.QDockWidget.__init__(self)
        # Set up the user interface from Designer.
        self.ui = Ui_IfsttarRoutingDock()
        self.ui.setupUi(self)

        self.criterionMap = { "Distance" : 1, "Duration" : 2, "Price" : 3, "Carbon" : 4, "Calories" : 5, "NumberOfChanges" : 6 }
        n = 0
        for k,v in self.criterionMap.items():
            self.ui.criterionList.insertItem( n, k )
            n += 1

        # preferences is an object used to store user preferences
        if os.path.exists( PREFS_FILE ):
            f = open( PREFS_FILE, 'r' )
            self.prefs = pickle.load( f )
            self.ui.wpsUrlText.setText( self.prefs['wps_url_text'] )
            self.ui.pluginCombo.setCurrentIndex( self.prefs['plugin_selected'] )
            self.ui.originText.setText( self.prefs['origin_text'] )
            self.ui.destinationText.setText( self.prefs['destination_text'] )
        else:
            self.prefs = {}

    def criterion_id( self, str ):
        return self.criterionMap[str]

    def closeEvent( self, event ):
        print "close !"
        self.prefs['wps_url_text'] = self.ui.wpsUrlText.text()
        self.prefs['plugin_selected'] = self.ui.pluginCombo.currentIndex()
        self.prefs['origin_text'] = self.ui.originText.text()
        self.prefs['destination_text'] = self.ui.destinationText.text()
        f = open( PREFS_FILE, 'w+' )
        pickle.dump( self.prefs, f )
        event.accept()
