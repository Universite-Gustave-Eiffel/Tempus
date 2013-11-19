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

import struct

class WKB:

    def __init__( self, wkb ):
        self.wkb = wkb
        # prefix for unpack : little or big endian
        self.prefix = '<' if wkb[1] == '1' else '>'
        self.type = struct.unpack(self.prefix+'h', self.wkb[1*2:3*2].decode('hex'))[0]

    # convert a LineStringZ to a 2D LineString
    def force2d( self ):
        if self.type != 1002: # LineStringZ
            return self.wkb
        nwkb = self.wkb[0:2] + '0200' + self.wkb[3*2:9*2]
        npts = struct.unpack(self.prefix+'h', self.wkb[5*2:7*2].decode('hex'))[0]
        for i in range(0,npts):
            p = 9+i*3*8
            xs = self.wkb[p*2:(p+8)*2]
            ys = self.wkb[(p+8)*2:(p+16)*2]
            nwkb += xs + ys
        return nwkb

    # get an array of (x,y,z) from a LineStringZ
    def dumpPoints( self ):
        if self.type != 1002:
            return []
        ret = []
        npts = struct.unpack(self.prefix+'h', self.wkb[5*2:7*2].decode('hex'))[0]
        for i in range(0,npts):
            p = 9+i*3*8
            xs = self.wkb[p*2:(p+8)*2]
            ys = self.wkb[(p+8)*2:(p+16)*2]
            zs = self.wkb[(p+16)*2:(p+24)*2]
            x = struct.unpack(self.prefix+'d', xs.decode('hex'))[0]
            y = struct.unpack(self.prefix+'d', ys.decode('hex'))[0]
            z = struct.unpack(self.prefix+'d', zs.decode('hex'))[0]
            ret.append( (x,y,z) )
        return ret
