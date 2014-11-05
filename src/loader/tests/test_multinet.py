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

class TestMultinetImporter(unittest.TestCase):

    def setUp(self):
        self.multinet = "%s/../../../data/multinet" % os.path.dirname(os.path.realpath(__file__))
        self.ml = tempus.MultinetImporter(self.multinet, "usaxxxxx______", "testmnet")

    def test_check_input(self):
        # test input checker 
        self.ml.check_input()

    def test_get_shapefiles(self):
        res = ['/home/vpicavet/oslandia/local/projets/tempus/data/multinet/usaxxxxx______nw.shp',
 '/home/vpicavet/oslandia/local/projets/tempus/data/multinet/usaxxxxx______jc.shp',
 '/home/vpicavet/oslandia/local/projets/tempus/data/multinet/usaxxxxx______mn.shp',
 '/home/vpicavet/oslandia/local/projets/tempus/data/multinet/usaxxxxx______cf.shp',
 '/home/vpicavet/oslandia/local/projets/tempus/data/multinet/usaxxxxx______2r.dbf']
        self.ml.get_shapefiles()
        self.assertEqual(self.ml.shapefiles, res)

    def test_load(self):
        ret = self.ml.load()
        self.assertEqual(ret, True)


if __name__ == '__main__':
    unittest.main()
