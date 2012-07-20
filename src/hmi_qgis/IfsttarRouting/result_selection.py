from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
import config

from ui_result_selection import Ui_ResultSelection

class ResultSelection( QWidget ):

    # class member for all the radio buttons
    buttonGroup = QButtonGroup()

    def __init__( self ):
        QWidget.__init__( self )
        self.widget = Ui_ResultSelection()
        self.widget.setupUi( self )
        self.nrows = 0
        ResultSelection.buttonGroup.addButton( self.widget.radioButton )

    def addCost( self, title, value ):
        lay = self.widget.fLayout
        lbl = QLabel()
        lbl.setText( title )
        vlbl = QLabel()
        vlbl.setText( value )
        lay.setWidget( self.nrows, QFormLayout.LabelRole, lbl )
        lay.setWidget( self.nrows, QFormLayout.FieldRole, vlbl )
        self.nrows += 1

    def id( self ):
        return ResultSelection.buttonGroup.id( self.widget.radioButton )

    def setText( self, text ):
        self.widget.radioButton.setText( text )
