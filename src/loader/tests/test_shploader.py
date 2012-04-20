#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader
# (c) 2012 Oslandia
# MIT Licence

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
