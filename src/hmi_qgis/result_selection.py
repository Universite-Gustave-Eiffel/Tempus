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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
import config

from ui_result_selection import Ui_ResultSelection

class ResultSelection( QWidget ):

    # class member for all the radio buttons
    buttonGroup = QButtonGroup()

    def __init__( self ):
        QWidget.__init__( self )
        self.widget = Ui_ResultSelection()
        self.widget.setupUi( self )
        self.nrows = 0
        ResultSelection.buttonGroup.addButton( self.widget.radioButton )

    def addCost( self, title, value ):
        lay = self.widget.fLayout
        lbl = QLabel()
        lbl.setText( title )
        vlbl = QLabel()
        vlbl.setText( value )
        lay.setWidget( self.nrows, QFormLayout.LabelRole, lbl )
        lay.setWidget( self.nrows, QFormLayout.FieldRole, vlbl )
        self.nrows += 1

    def id( self ):
        return ResultSelection.buttonGroup.id( self.widget.radioButton )

    def setText( self, text ):
        self.widget.radioButton.setText( text )
