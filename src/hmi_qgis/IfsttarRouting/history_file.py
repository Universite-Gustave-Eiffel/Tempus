# -*- coding: utf-8 -*-
"""
/**
 * 
 * History file management
 *
 */
"""

import sqlite3
import os
from datetime import datetime
from zipfile import *
import tempfile

class HistoryFile(object):

    def __init__( self, filename ):
        self.filename = filename
        if not os.path.exists( filename ):
            self.connection = sqlite3.connect( filename )
            cursor = self.connection.cursor()
            cursor.execute( 'CREATE TABLE history ( id integer primary key autoincrement, date text, value text )')
            self.connection.commit()
            cursor.close()
        else:
            self.connection = sqlite3.connect( filename )

    def addRecord( self, record ):
        cursor = self.connection.cursor()
        self.connection.text_factory = str
        cursor.execute( "INSERT INTO history (date, value) VALUES (?, ?)", ( datetime.isoformat(datetime.now()), record ) )
        self.connection.commit()
        cursor.close()

    def removeRecord( self, id ):
        cursor = self.connection.cursor()
        cursor.execute( "DELETE from history WHERE id='%d'" % id)
        self.connection.commit()
        cursor.close()

    def getRecordsSummary( self ):
        cursor = self.connection.cursor()
        cursor.execute( "SELECT id, date from history ORDER BY date DESC" )
        r = cursor.fetchall()
        cursor.close()
        return r

    def getRecord( self, id ):
        cursor = self.connection.cursor()
        self.connection.text_factory = str
        cursor.execute( "SELECT * from history WHERE id='%d'" % id)
        r = cursor.fetchone()
        cursor.close()
        return r

class ZipHistoryFile(object):

    def __init__( self, filename ):
        self.filename = filename

        self.zf = None
        self.reset()

    def __del__( self ):
        self.zf.close()

    def reset( self ):
        if self.zf:
            self.zf.close()
        self.zf = ZipFile( self.filename, 'a')
        self.lastid = 0
        for info in self.zf.infolist():
            n = int(info.filename)
            if n > self.lastid:
                self.lastid = n

    def addRecord( self, record ):
        f = tempfile.NamedTemporaryFile( delete = False)
        f.write( record.encode('utf-8') )
        f.close()
        self.lastid = self.lastid+1
        n = "%d" % self.lastid
        self.zf.write( f.name, n )

        self.reset()

    def removeRecord( self, id ):
        # must copy to a new file
        self.zf.close()

        zf = ZipFile( self.filename, 'r' )
        newf = self.filename + ".new"
        zf2 = ZipFile( newf, 'w' )
        for info in zf.infolist():
            # copy all except the given id
            if info.filename != str(id):
                zf2.writestr( info, zf.read( info.filename ) )
        zf.close()
        zf2.close()

        os.unlink( self.filename )
        os.rename( newf, self.filename )
        # reopen the new file
        self.zf = ZipFile( self.filename, 'a' )
        # last id stays the same

    def getRecordsSummary( self ):
        self.reset()
        infos = self.zf.infolist()
        r = []
        for info in infos:
            t = info.date_time
            dt = "%04d-%02d-%02dT%02d:%02d:%02d.0" % t
            r.append( [int(info.filename), dt] )
        return r

    def getRecord( self, id ):
        f = str(id)
        info = self.zf.getinfo( f )
        dt =  "%04d-%02d-%02dT%02d:%02d:%02d.0" % info.date_time
        ff = self.zf.open( f, 'r' )
        tt = ff.read()
        return (id, dt, tt )
