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

        begin                : 2012-05-28
        copyright            : (C) 2012 by Hugo Mercier/Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/
 This script initializes the plugin, making it known to QGIS.
"""
def name():
    return "Intralys display plugin"
def description():
    return "Allows automatic refresh of the canvas"
def version():
    return "Version 1.0"
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
