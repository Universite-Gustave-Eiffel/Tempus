@echo off

ECHO activating VS command prompt
IF /I "%platform%"=="x64" ECHO x64 && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
IF /I "%platform%"=="x86" ECHO x86 && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

set BOOST_ROOT=C:\Libraries\boost_1_59_0
set BOOST_LIBRARYDIR=C:\Libraries\boost_1_59_0\lib64-msvc-14.0

echo Running cmake...
cd c:\projects\tempus
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_DOC=OFF -DBUILD_OSM2SHP=OFF -DBUILD_QGIS_PLUGIN=OFF -DBUILD_WPS=OFF

echo Populating test db ...
psql -U postgres -c "create database tempus_test_db;"
psql -U postgres -c "create extension postgis;" tempus_test_db
7z x data\tempus\tempus_test_db\tempus_test_db.sql.zip -o tempus_test_db.sql -y
psql -U postgres tempus_test_db < tempus_test_db.sql
psql -U postgres tempus_test_db < data\tempus\tempus_test_db\patch.001.sql
psql -U postgres tempus_test_db < data\tempus\tempus_test_db\patch.002.sql
psql -U postgres tempus_test_db < data\tempus\tempus_test_db\patch.003.sql

nmake

GOTO DONE



:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO build started^: %NODE_GDAL_BUILD_START_TIME%
ECHO build finished^: %NODE_GDAL_BUILD_FINISH_TIME%

EXIT /b %EL%