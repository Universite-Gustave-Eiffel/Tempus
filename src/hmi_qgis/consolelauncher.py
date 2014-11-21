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

/***************************************************************************
 IfsttarRouting
                                 A QGIS plugin
 Get routing informations with various algorithms
                              -------------------
        begin                : 2012-04-12
        copyright            : (C) 2012-2013 by Oslandia
        email                : hugo.mercier@oslandia.com
 ***************************************************************************/
"""

import sys
import os
import subprocess
import tempfile
import signal
from PyQt4.QtCore import *
from PyQt4.QtGui import *

# class used to redirect stdout to a TextEdit
class RedirectOnTextEdit:
    def __init__( self, widget ):
        self.widget = widget
        self.txt = ''

    def write(self, s):
        self.txt += s
        self.widget.setText(self.txt)
        c = self.widget.textCursor()
        c.movePosition( QTextCursor.End )
        self.widget.setTextCursor(c)
        QCoreApplication.processEvents()


class TextOutputThread(QThread):

    new_line = pyqtSignal()

    def __init__( self, stdout, mutex, queue ):
        QThread.__init__( self )
        self.stdout = stdout
        self.txt = ''
        self.mutex = mutex
        self.queue = queue

    def run( self ):
        while True:
            line = self.stdout.readline()
            if line is None or len(line) == 0:
                self.stdout.close()
                self.quit()
                return
            self.mutex.lock()
            self.queue.append(line)
            self.mutex.unlock()
            self.new_line.emit()
            

# Sub process launcher working on Linux / Windows
# With stdout/stderr redirection in a Qt-based console
class ConsoleLauncher:

    def __init__( self, cmd ):
        self.cmd = cmd
        self.proc = None

        # a very simple dialog
        self.dlg = QDialog( None )
        self.dlg.finished.connect( self.stop )
        self.layout = QVBoxLayout()
        self.textEdit = QTextEdit( self.dlg )
        self.layout.addWidget( self.textEdit )
        self.dlg.setLayout( self.layout )
        self.dlg.resize( 800, 200 )

        self.txt = ''

        self.mutex = QMutex()
        self.queue = []
        self.reentrant = False

    def launch( self ):
        self.stop()

        opts = {}
        if sys.platform == 'win32':
            # close_fds = True to avoid inheriting file handles
            self.proc = subprocess.Popen(self.cmd, close_fds = True )
        else:
            # os.setid: run subprocess in a process group
            # so that we can kill it afterward
            self.proc = subprocess.Popen(self.cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, preexec_fn = os.setsid)

            # redirect stdout/err to a QDialog
            self.thread = TextOutputThread( self.proc.stdout, self.mutex, self.queue )
            self.thread.new_line.connect( self.on_new_line )
            self.thread.start()
            self.thread2 = TextOutputThread( self.proc.stderr, self.mutex, self.queue )
            self.thread2.new_line.connect( self.on_new_line )
            self.thread2.start()
        
            self.dlg.show()

    def on_new_line( self ):
        if self.reentrant:
            return
        self.reentrant = True

        self.mutex.lock()
        line = ''.join(self.queue)
        while len(self.queue):
            self.queue.pop()
        self.mutex.unlock()

        self.txt += line
        self.textEdit.setText(self.txt)
        c = self.textEdit.textCursor()
        c.movePosition( QTextCursor.End )
        self.textEdit.setTextCursor(c)
        QCoreApplication.processEvents()

        self.reentrant = False

    def stop( self ):
        if self.proc is not None:
            if sys.platform == 'win32':
                 self.proc.terminate()
                 self.proc.wait()
            else:
                os.killpg(self.proc.pid, signal.SIGTERM)
                self.thread.wait()
                self.thread2.wait()
            self.proc = None

if __name__ == "__main__":
    app = QApplication(sys.argv)
    c = ConsoleLauncher(['find', '/'])
    c.launch()
    app.exec_()
