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
 Intralys
                                 A QGIS plugin
 Allows automatic refresh of the canvas
                              -------------------
        begin                : 2012-05-28
        copyright            : (C) 2012 by Hugo Mercier/Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/
"""
# Import the PyQt and QGIS libraries
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
import PyQt4.QtCore
import PyQt4.QtGui
# Initialize Qt resources from file resources.py
import resources_rc
# Import the code for the dialog
from intralysdialog import IntralysDialog

class Intralys:

    def __init__(self, iface):
        # Save reference to the QGIS interface
        self.iface = iface
        # Create the dialog and keep reference
        self.dlg = IntralysDialog()
        # initialize plugin directory
        self.plugin_dir = QFileInfo(QgsApplication.qgisUserDbFilePath()).path() + "/python/plugins/intralys"
        # initialize locale
        localePath = ""
        locale = QSettings().value("locale/userLocale")[0:2]
       
        if QFileInfo(self.plugin_dir).exists():
            localePath = self.plugin_dir + "/i18n/intralys_" + locale + ".qm"

        if QFileInfo(localePath).exists():
            self.translator = QTranslator()
            self.translator.load(localePath)

            if qVersion() > '4.3.3':
                QCoreApplication.installTranslator(self.translator)
   

    def initGui(self):
        # Create action that will start plugin configuration
        self.action = QAction(QIcon(":/plugins/intralys/icon.png"), \
            u"Canvas auto-refresh", self.iface.mainWindow())
        # connect the action to the run method
        QObject.connect(self.action, SIGNAL("triggered()"), self.run)

        # Add toolbar button and menu item
        self.iface.addToolBarIcon(self.action)
        self.iface.addPluginToMenu(u"&Intralys display", self.action)

        self.is_fullscreen = False
        self.fsAction = QAction( "Canvas in full screen", self.dlg )
        self.fsAction.setShortcut( "F11" )
        self.timer = None
        viewMenu = self.iface.viewMenu()
        viewMenu.addAction( self.fsAction )
        QObject.connect( self.fsAction, SIGNAL("triggered()"), self.toggleFS )


    def unload(self):
        # Remove the plugin menu item and icon
        self.iface.removePluginMenu(u"&Intralys display",self.action)
        self.iface.removeToolBarIcon(self.action)
        self.iface.viewMenu().removeAction( self.fsAction )

    def backFromFullScreen( self ):
        if not self.is_fullscreen:
            return

        win = self.iface.mainWindow()
        win.restoreState( self.saved_state )
        win.showNormal()

        self.is_fullscreen = False

    def setFullScreen( self ):
        if self.is_fullscreen:
            return

        win = self.iface.mainWindow()
        self.saved_state = win.saveState()
            
        children = win.findChildren( PyQt4.QtGui.QToolBar )
        for child in children:
            win.removeToolBar( child )
                
        children = win.findChildren( PyQt4.QtGui.QDockWidget )
        for child in children:
            child.hide()

        win.showFullScreen()
        self.is_fullscreen = True

    def toggleFS( self ):
        if not self.is_fullscreen:
            self.setFullScreen()
        else:
            self.backFromFullScreen()

    def update(self):
#        print "Update()"
        self.iface.mapCanvas().refresh()

    # run method that performs all the real work
    def run(self):
        # show the dialog
        self.dlg.show()
        # Run the dialog event loop
        result = self.dlg.exec_()
        # See if OK was pressed
        if result == 1:
            if self.timer is not None:
                self.timer.stop()

            self.timer = QTimer( self.dlg )
            QObject.connect( self.timer, SIGNAL("timeout()"), self.update )
            rate = int(self.dlg.ui.rateEdit.text()) * 1000

            if rate == 0:
                return
            if rate < 200:
                rate = 1000
            self.timer.start( rate )
