# -*- coding: utf-8 -*-
"""
/***************************************************************************
 Intralys
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
 This script initializes the plugin, making it known to QGIS.
"""
def name():
    return "Intralys display plugin"
def description():
    return "Allows automatic refresh of the canvas"
def version():
    return "Version 0.1"
def icon():
    return "icon.png"
def qgisMinimumVersion():
    return "1.8"
def qgisMaximumVersion():
    return "2.9"
def classFactory(iface):
    # load Intralys class from file Intralys
    from intralys import Intralys
    return Intralys(iface)
