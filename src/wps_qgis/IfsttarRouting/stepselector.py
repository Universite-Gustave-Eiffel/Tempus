from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
import config

class StepSelector( QWidget ):

    def __init__( self, parent, name = "Origin" ):
        QWidget.__init__( self )
        self.parent = parent

        self.layout = QVBoxLayout( self )
        self.layout.setMargin( 0 )

        self.label = QLabel( self )
        self.label.setText( name )

        self.layout.addWidget( self.label )

        self.hlayout = QHBoxLayout( self )
        self.hlayout.setMargin( 0 )
        self.layout.addLayout( self.hlayout )

        self.coordinates = QLineEdit()
        self.selectBtn = QToolButton()
        self.selectBtn.setIcon( QIcon( config.DATA_DIR + "/mouse_cross.png" ) )

        self.hlayout.addWidget( self.coordinates )
        self.hlayout.addWidget( self.selectBtn )

        if name == "Destination":
            self.plusBtn = QToolButton()
            self.plusBtn.setIcon( QIcon.fromTheme("add") )
            self.hlayout.addWidget( self.plusBtn )
            QObject.connect( self.plusBtn, SIGNAL("clicked()"), self.onAdd )
        else:
            if name != "Origin":
                self.minusBtn = QToolButton()
                self.minusBtn.setIcon( QIcon.fromTheme("remove") )
                self.hlayout.addWidget( self.minusBtn )
                QObject.connect( self.minusBtn, SIGNAL("clicked()"), self.onRemove )

    def set_canvas( self, canvas ):
        self.canvas = canvas

        self.clickTool = QgsMapToolEmitPoint( self.canvas )
        QObject.connect( self.selectBtn, SIGNAL("clicked()"), self.onSelect )

    def get_coordinates( self ):
        s = self.coordinates.text().split(',')
        if len(s) == 2:
            return [ float(s[0]), float(s[1]) ]
        return [ 0, 0 ]

    def set_coordinates( self, xy ):
        self.coordinates.setText( "%f, %f" % ( xy[0], xy[1] ) )

    def onAdd( self ):
        # we assume the parent widget is a QLayout
        s = StepSelector( self.parent, "Step" )
        s.set_canvas( self.canvas )

        # remove the last one
        lw = self.parent.itemAt( self.parent.count() - 1 ).widget()
        self.parent.removeWidget( lw )
        self.parent.addWidget( s )
        # add back the last one
        self.parent.addWidget( lw )

    def onRemove( self ):
        self.parent.removeWidget( self )
        self.close()

    def onSelect( self ):
        print "onSelect"
        QObject.connect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onCanvasClick)
        self.canvas.setMapTool(self.clickTool)

    def onCanvasClick( self, point, button ):
        geom = QgsGeometry.fromPoint(point)
        p = geom.asPoint()
        print "Selection Changed", point.x(), point.y()
        self.canvas.unsetMapTool( self.clickTool )
        self.set_coordinates( [p.x(), p.y()] )
        QObject.disconnect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onCanvasClick)


