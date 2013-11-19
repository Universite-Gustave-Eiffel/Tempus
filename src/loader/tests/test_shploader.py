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

#
# Tempus data loader

import unittest
import tempus
import os

class TestShpLoader(unittest.TestCase):

    def setUp(self):
        self.shp = "%s/../../../data/point.shp" % os.path.dirname(os.path.realpath(__file__))
        self.sloader = tempus.ShpLoader()
        self.sloader.set_shapefile(self.shp)
        self.sloader.set_table("pointtest")
        self.sloader.set_schema("public")
        self.sloader.set_options({'I':True})
        dbstring = "host=localhost dbname=tempus user=postgres port=5432"
        self.sloader.set_dbparams(dbstring)

    def test_shp2pgsql(self):
        # test shp2pgsql output
        self.sloader.shp2pgsql()
        sql = open("point_output.sql").read()
        output = open(self.sloader.sqlfile).read()
        self.assertEqual(output, sql)
        self.sloader.clean()

if __name__ == '__main__':
    unittest.main()
