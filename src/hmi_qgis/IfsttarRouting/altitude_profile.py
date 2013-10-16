from PyQt4.QtCore import *
from PyQt4.QtGui import *

class AltitudeProfile(QGraphicsView):
    def __init__( self, parent = None ):
        QGraphicsView.__init__( self, parent )
        self.setScene( AltitudeProfileScene(parent) )
        # enable mousmove when no mouse button is pressed
        self.setMouseTracking( True )
        # set fixed height and scrollbat policies
#        self.setFixedHeight(75)
        self.setVerticalScrollBarPolicy( Qt.ScrollBarAlwaysOff )
        self.setHorizontalScrollBarPolicy( Qt.ScrollBarAlwaysOff )

        self.xRatio = 1.0
        self.yRatio = 1.0

    def autoFit( self ):
        r = self.scene().sceneRect()
        # add 10% of height
        h = r.height() * 0.2
        r.setHeight( r.height() + h )
        r.setTop( r.top() + h/2 )
        self.fitInView( r )

    def addElevation( self, distance, z0, z1 ):
        self.scene().addElevation( distance, z0, z1 )

    def resizeEvent( self, event ):
        QGraphicsView.resizeEvent( self, event )
        #print "resize to %d %d" % (event.size().width(), event.size().height())
        self.autoFit()
        r = self.scene().sceneRect()
        self.xRatio = r.width() / event.size().width()
        self.yRatio = r.height() / event.size().height()

    def mouseMoveEvent( self, event ):
        (x,y) = (event.x(), event.y())
        pt = self.mapToScene( x, y )
        self.scene().onMouseOver( pt, self.xRatio, self.yRatio )

class AltitudeProfileScene(QGraphicsScene):

    def __init__( self, parent ):
        QGraphicsScene.__init__( self, parent )
        self.x = 0
        self.alts = []
        self.circle = None

    def clear( self ):
        QGraphicsScene.clear( self )
        self.alts = []

    def addElevation( self, distance, z0, z1 ):
        self.addLine( self.x, z0, self.x+distance, z1 )
        self.alts.append( (self.x, self.x+distance, z0, z1) )
        self.x += distance

    # called to add a circle on top of the current altitude profile
    # point : QPointF of the mouse position (in scene coordinates)
    # xRatio, yRatio : factors to convert from view coordinates to scene coordinates
    def onMouseOver( self, point, xRatio, yRatio ):
        x = point.x()

        if self.circle:
            self.removeItem( self.circle )
            self.circle = None

        # look for altitude
        for (ax1, ax2, az1, az2) in self.alts:
            if ax1 <= x < ax2:
                y = ((x-ax1)/(ax2-ax1)) * (az2-az1) + az1
                w = 4*xRatio
                h = 4*yRatio
                self.circle = self.addEllipse( x-w, y-h, w*2, h*2)
                break
