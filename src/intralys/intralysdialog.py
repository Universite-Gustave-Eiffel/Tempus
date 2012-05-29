# -*- coding: utf-8 -*-
"""
/***************************************************************************
 IntralysDialog
                                 A QGIS plugin
 Allows automatic refresh of the canvas
                             -------------------
        begin                : 2012-05-28
        copyright            : (C) 2012 by Hugo Mercier/Oslandia
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
from ui_intralys import Ui_Intralys
# create the dialog for zoom to point
class IntralysDialog(QtGui.QDialog):
    def __init__(self):
        QtGui.QDialog.__init__(self)
        # Set up the user interface from Designer.
        self.ui = Ui_Intralys()
        self.ui.setupUi(self)
