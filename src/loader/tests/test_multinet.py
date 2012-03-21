#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

import unittest
import tempus
import os

class TestMultinetLoader(unittest.TestCase):

    def setUp(self):
        self.multinet = "%s/../../../data/multinet" % os.path.dirname(os.path.realpath(__file__))
        self.ml = tempus.MultinetLoader(self.multinet, "usaxxxxx______", "testmnet")

    def test_check_input(self):
        # test input checker 
        res = self.ml.check_input()
        self.assertEqual(res, True)

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
