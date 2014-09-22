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
import config

class CriterionChooser( QWidget ):

    criterion_id = { "Distance" : 1, "Duration" : 2, "Price" : 3, "Carbon" : 4, "Calories" : 5, "NumberOfChanges" : 6, "Variability" : 7 }

    def __init__( self, parent, first = False ):
        QWidget.__init__( self )
        self.parent = parent

        self.layout = QHBoxLayout( self )
        self.layout.setMargin( 0 )

        self.label = QLabel( self )
        self.label.setText( "Criterion" )

        self.criterionList = QComboBox( self )        
        n = 0
        for k,v in CriterionChooser.criterion_id.items():
            self.criterionList.insertItem( n, k )
            n += 1

        self.btn = QToolButton( self )
        if first:
            self.btn.setIcon( QIcon( config.DATA_DIR + "/add.png" ) )
            QObject.connect( self.btn, SIGNAL("clicked()"), self.onAdd )
        else:
            self.btn.setIcon( QIcon( config.DATA_DIR + "/remove.png" ) )
            QObject.connect( self.btn, SIGNAL("clicked()"), self.onRemove )

        self.layout.addWidget( self.label )
        self.layout.addWidget( self.criterionList )
        self.layout.addWidget( self.btn )

    def onAdd( self ):
        # daddy is supposed to be a QLayout here
        self.parent.addWidget( CriterionChooser( self.parent ) )

    def onRemove( self ):
        self.parent.removeWidget( self )
        self.close()
        self.parent.update()

    def selected( self ):
        return CriterionChooser.criterion_id[ str(self.criterionList.currentText()) ]

    def set_selection( self, selection_id ):
        # look for this ID
        name = ''
        for k,nid in CriterionChooser.criterion_id.items():
            if nid == selection_id:
               name = k
               break
        # select based on a name
        for i in range(0, self.criterionList.count()):
            t = self.criterionList.itemText( i )
            if t == name:
                self.criterionList.setCurrentIndex( i )
                break

    def set_supported_criteria( self, criteria ):
        n = 0
        self.criterionList.clear()
        for t, i in CriterionChooser.criterion_id.iteritems():
            if i in criteria:
                self.criterionList.insertItem( n, t )
                n += 1
                continue
