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
 IfsttarRouting
                                 A QGIS plugin
 Get routing informations with various algorithms
                             -------------------
        begin                : 2012-04-12
        copyright            : (C) 2012 by Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/

 This script initializes the plugin, making it known to QGIS.
"""
def name():
    return "Ifsttar routing plugin"
def description():
    return "Get routing informations with various algorithms"
def version():
    return "Version 0.1"
def icon():
    return "icon.png"
def qgisMinimumVersion():
    return "1.8"
def qgisMaximumVersion():
    return "2.9"
def classFactory(iface):
    # load IfsttarRouting class from file IfsttarRouting
    from ifsttarrouting import IfsttarRouting
    return IfsttarRouting(iface)
