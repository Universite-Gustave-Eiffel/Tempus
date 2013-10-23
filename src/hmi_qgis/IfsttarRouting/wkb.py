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
