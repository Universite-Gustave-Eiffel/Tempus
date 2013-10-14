from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from datetime import datetime
import config

class StepSelector( QFrame ):

    def __init__( self, parent, name = "Origin", coordinates_only = False ):
        QFrame.__init__( self )
        self.parent = parent

        self.layout = QVBoxLayout( self )
        self.layout.setMargin( 0 )

        self.label = QLabel()
        self.label.setText( name )

        self.layout.addWidget( self.label )

        self.hlayout = QHBoxLayout()
        self.hlayout.setMargin( 0 )
        self.layout.addLayout( self.hlayout )

        self.coordinates = QLineEdit()
        self.selectBtn = QToolButton()
        self.selectBtn.setIcon( QIcon( config.DATA_DIR + "/mouse_cross.png" ) )

        self.hlayout.addWidget( self.coordinates )
        self.hlayout.addWidget( self.selectBtn )

        self.setLayout( self.layout )
        self.setFrameStyle( QFrame.Box )

        self.pvadCheck = None
        if coordinates_only:
            return

        if name == "Destination":
            self.plusBtn = QToolButton()
            self.plusBtn.setIcon( QIcon( config.DATA_DIR + "/add.png" ) )
            self.hlayout.addWidget( self.plusBtn )
            QObject.connect( self.plusBtn, SIGNAL("clicked()"), self.onAdd )
        else:
            if name != "Origin":
                self.minusBtn = QToolButton()
                self.minusBtn.setIcon( QIcon( config.DATA_DIR + "/remove.png" ) )
                self.hlayout.addWidget( self.minusBtn )
                QObject.connect( self.minusBtn, SIGNAL("clicked()"), self.onRemove )

        n = datetime.now()
        self.dateEdit = QDateTimeEdit( QDateTime.currentDateTime(), self )
        self.dateEdit.setCalendarPopup( True )
        self.constraintBox = QComboBox()
        self.constraintBox.insertItem(0, "No constraint" )
        self.constraintBox.insertItem(1, "Before" )
        self.constraintBox.insertItem(2, "After" )

        self.hlayout2 = QHBoxLayout()
        self.hlayout2.setMargin( 0 )
        self.hlayout2.addWidget( self.constraintBox )
        self.hlayout2.addWidget( QLabel("Date and time") )
        self.hlayout2.addWidget( self.dateEdit )

        self.layout.addLayout( self.hlayout2 )

        if name != 'Origin':
            self.pvadCheck = QCheckBox( "Private vehicule at destination" )
            self.layout.addWidget( self.pvadCheck )
            
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
        if not xy:
            self.coordinates.setText("-- unavailable --")
            return
        if xy == []:
            xy = [ 0, 0 ]
        self.coordinates.setText( "%f, %f" % ( xy[0], xy[1] ) )

    def get_constraint_type( self ):
        return self.constraintBox.currentIndex()

    def set_constraint_type( self, idx ):
        self.constraintBox.setCurrentIndex( idx )

    def get_constraint( self ):
        return self.dateEdit.dateTime().toString(Qt.ISODate)

    def set_constraint( self, str ):
        datetime = QDateTime.fromString( str, Qt.ISODate )
        self.dateEdit.setDateTime( datetime )

    def get_pvad( self ):
        if self.pvadCheck is None:
            return False
        return self.pvadCheck.checkState() == Qt.Checked

    def set_pvad( self, check ):
        if self.pvadCheck is None:
            return
        state = Qt.Checked
        if check == False:
            state = Qt.Unchecked
        self.pvadCheck.setCheckState( state )

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
        QObject.connect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onCanvasClick)
        self.canvas.setMapTool(self.clickTool)

    def onCanvasClick( self, point, button ):
        geom = QgsGeometry.fromPoint(point)
        p = geom.asPoint()
        self.canvas.unsetMapTool( self.clickTool )
        self.set_coordinates( [p.x(), p.y()] )
        QObject.disconnect(self.clickTool, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.onCanvasClick)


