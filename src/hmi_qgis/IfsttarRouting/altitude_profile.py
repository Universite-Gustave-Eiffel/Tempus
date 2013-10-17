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

    def clear( self ):
        self.scene().clear()

    def addElevation( self, distance, z0, z1, section ):
        self.scene().addElevation( distance, z0, z1, section )

    # sceneRect is always set to the resize event size
    # this way, 1 pixel in the scene is 1 pixel on screen
    def resizeEvent( self, event ):
        QGraphicsView.resizeEvent( self, event )
        r = self.scene().sceneRect()
        self.scene().setSceneRect( QRectF( 0, 0, event.size().width(), event.size().height() ) )
        self.displayElevations()

    def displayElevations( self ):
        self.scene().displayElevations()

    def highlightSelection( self, section ):
        self.scene().highlightSelection( section )

    def mouseMoveEvent( self, event ):
        (x,y) = (event.x(), event.y())
        pt = self.mapToScene( x, y )
        self.scene().onMouseOver( pt )

class AltitudeProfileScene(QGraphicsScene):

    def __init__( self, parent ):
        QGraphicsScene.__init__( self, parent )
        # width of the scale bar
        fm = QFontMetrics(QFont())
        # width = size of "100.0" with the default font + 10%
        self.barWidth = fm.width("Z (m) ") * 1.10
        self.xOffset = self.barWidth
        self.yOffset = fm.height() * 2

        # altitude marker
        self.marker = AltitudeMarker( self )

        # define the transformation between distance, altitude and scene coordinates
        self.xRatio = 1.0
        self.yRatio = 1.0

        self.clear()

    def clear( self ):
        QGraphicsScene.clear( self )
        self.x = 0
        self.alts = []

        # define the altitude profile rect
        self.yMin = 10000.0
        self.yMax = 0.0
        self.xMax = 0.0

        self.selection = []

    def setSceneRect( self, rect ):
        QGraphicsScene.setSceneRect( self, rect )
        w = rect.width() - self.barWidth
        self.xRatio = w / self.xMax
        h = rect.height() - self.yOffset * 2
        self.yRatio = h / (self.yMax-self.yMin)

    # convert distance to scene coordinate
    def xToScene( self, x ):
        return x * self.xRatio + self.xOffset
    # convert altitude to scene coordinate
    def yToScene( self, y ):
        return self.sceneRect().height() - ((y - self.yMin)* self.yRatio + self.yOffset)

    def addElevation( self, distance, z0, z1, section ):
        self.alts.append( (self.x, self.x+distance, z0, z1, section) )
        self.yMin = min(self.yMin, z0)
        self.yMax = max(self.yMax, z1)
        self.x += distance
        self.xMax = self.x

    def displayElevations( self ):
        QGraphicsScene.clear( self )
        self.marker.clear()
        self.selection = []
        r = self.sceneRect();

        # display lines fitting in sceneRect
        poly = QPolygonF()
        x1 = self.alts[0][0]
        y1 = self.alts[0][2]
        poly.append( QPointF(self.xToScene(x1), self.yToScene(y1)) )
        for (x1,x2,y1,y2, section) in self.alts:
            poly.append( QPointF(self.xToScene(x2), self.yToScene(y2)) )
        # close the polygon
        x2 = self.xToScene( self.xMax )
        y2 = self.sceneRect().height()
        poly.append( QPointF(x2, y2) )
        x2 = self.barWidth
        poly.append( QPointF(x2, y2) )
        x2 = self.xToScene(self.alts[0][0])
        y2 = self.yToScene(self.alts[0][2])
        poly.append( QPointF(x2, y2) )
        brush = QBrush( QColor( 204, 255, 153 ) )
        pen = QPen()
        pen.setWidth(0)
        self.addPolygon( poly, pen, brush )

        # horizontal line on ymin and ymax
        self.addLine( self.barWidth-5, self.yToScene(self.yMin), self.barWidth+5, self.yToScene(self.yMin) );
        self.addLine( self.barWidth-5, self.yToScene(self.yMax), self.barWidth+5, self.yToScene(self.yMax) );

        # display scale
        self.addLine( self.barWidth, 0, self.barWidth, self.sceneRect().height() )

        font = QFont()
        fm = QFontMetrics( font )
        t1 = self.addText( "%.1f" % self.yMin )
        t1.setPos( 0, self.yToScene(self.yMin)-fm.ascent())
        t2 = self.addText( "%.1f" % self.yMax )
        t2.setPos( 0, self.yToScene(self.yMax)-fm.ascent())

        # Z(m)
        t3 = self.addText( "Z (m)" )
        t3.setPos( 0, 0 )

    # called to add a circle on top of the current altitude profile
    # point : QPointF of the mouse position (in scene coordinates)
    # xRatio, yRatio : factors to convert from view coordinates to scene coordinates
    def onMouseOver( self, point ):
        x = point.x()

        # look for altitude
        for (ax1, ax2, az1, az2, section) in self.alts:
            ax1 = self.xToScene( ax1 )
            ax2 = self.xToScene( ax2 )
            if ax1 <= x < ax2:
                z = ((x-ax1)/(ax2-ax1)) * (az2-az1) + az1
                y = self.yToScene( z )
                self.marker.setText( "%.1f" % z )
                self.marker.moveTo( x, y )
                break

    # highlight a given section number
    def highlightSelection( self, section ):
        for s in self.selection:
            self.removeItem(s)
        self.selection = []

        for s in section:
            for alt in self.alts:
                x1 = alt[0]
                x2 = alt[1]
                ss = alt[4]
                if ss == s:
                    w = self.xToScene(x2)-self.xToScene(x1)
                    brush = QBrush( QColor(120,120,120,120))
                    pen = QPen()
                    pen.setStyle( Qt.NoPen )
                    self.selection.append(self.addRect(QRectF(self.xToScene(x1),0,w,self.sceneRect().height()),pen,brush))

# graphics items that are displayed on mouse move on the altitude curve
class AltitudeMarker:
    def __init__( self, scene ):
        # circle item
        self.circle = None
        # text item
        self.text = None
        # the graphics scene
        self.scene = scene
        # a line
        self.line = None

    def clear( self ):
        self.circle = None
        self.text = None
        self.line = None

    def setText( self, text ):
        if not self.text:
            font = QFont()
            font.setPointSize(10)
            self.text = self.scene.addText("", font)
        self.text.setPlainText( text )

    def moveTo( self, x, y ):
        if not self.circle:
            brush = QBrush( QColor( 0, 0, 200 ) ) # blue brush
            self.circle = self.scene.addEllipse( 0,0,4,4,QPen(),brush)
        if not self.line:
            pen = QPen()
            pen.setStyle( Qt.DashLine )
            self.line = self.scene.addLine( 0,0,0,0,pen)

        self.circle.setRect( x-4, y-4, 8, 8 )
        self.line.setLine( x, 0, x, self.scene.sceneRect().height() )
        if self.text:
            self.text.setPos( x, y )

