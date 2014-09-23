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

class PolygonSelection( QgsMapTool ):

    polygonCaptured = pyqtSignal(bool)

    def __init__( self, canvas ):
        self.canvas = canvas
        QgsMapTool.__init__( self, self.canvas )
        self.rubber = QgsRubberBand( self.canvas, QGis.Polygon )
        self.rubber.setWidth(2)
        self.rubber.setColor(QColor(0,0,255,120))
        self.currentPoint = -1
        self.inProgress = False
        # signal : False: canceled, True: ok
        self.polygon = None

    def activate( self ):
        self.inProgress = True
        self.rubber.reset( QGis.Polygon )
        self.currentPoint = -1
        self.setCursor( Qt.CrossCursor )
        QgsMapTool.activate( self )

    def canvasPressEvent( self, e ):
        if not self.inProgress:
            return
        if e.button() == Qt.LeftButton:
            p = self.canvas.getCoordinateTransform().toMapCoordinates( e.pos() )
            self.rubber.addPoint( p )
            self.currentPoint += 1
        elif e.button() == Qt.RightButton:
            self.polygonCaptured.emit( False )
            self.clear()

    def canvasMoveEvent( self, e ):
        if not self.inProgress:
            return
        if self.currentPoint >= 0:
            p = self.canvas.getCoordinateTransform().toMapCoordinates( e.pos() )
            self.rubber.movePoint( p )

    def canvasDoubleClickEvent( self, e ):
        if not self.inProgress:
            return
        if self.currentPoint >= 0:
            self.polygon = self.rubber.asGeometry().exportToWkt()
            self.polygonCaptured.emit(True)
            self.clear()

    def clear( self ):
        self.currentPoint = -1
        self.rubber.reset()
        self.inProgress = False







        
