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
