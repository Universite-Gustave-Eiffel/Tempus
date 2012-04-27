WIN32 compilation and installation
==================================

List of versions used :
- Visual Studio 2008 Express
- Microsoft windows Server 2003 SP1 Platform SDK
- FastCGI 2.4.1-SNAP-0910052249
- CppUnit 1.12.1
- libxml2 2.7.8.win32 from ftp://ftp.zlatkovic.com/libxml/
- iconv 1.9.2.win32 from ftp://ftp.zlatkovic.com/libxml/
- CMake 2.8.7

libxml2
=======

- Uncompress the .zip archives

FastCGI
=======

- Launch the "Visual Studio 2008 Command Prompt"
- Change the directory to the one that holds FastCGI sources
- Run "nmake /F Makefile.nt"
- It should generates libraries in libfgci\Debug and libfcgi\Release with lots of (hopefully) harmless warnings

CppUnit
=======

- Open the Visual Studio project in src\CppUnitLibraries.dsw
- If asked for conversion to the new format, answer Yes to All
- Use the Configuration Manager to set the current configuration to "Release".
- Build project "cppunit" and "cppunit_dll"
- .lib and .dll should be present in the lib\ directories

Tempus compilation
==================

- Launch a "Visual Studio 2008 Command Prompt"
- Go to the Tempus src/ directory
- Create a 'build' subdirectory
- Enter this directory

- If you also want to compile the QGIS plugin, you should be able to call python, pyuic4 and pyrcc4 from the command line
( If Python and libs have been installed via OsGeo4W, just execute OSGEO4W_INSTALL_DIR\Osgeo4w.bat )
  - Set PYUIC4_PROGRAM to the path of 'pyuic4'
  - Set PYRCC4_PROGRAM to the path of 'pyrcc4'

- Run 'cmake-gui ..' (must be in the path)
- When clicking on "Configure", choose "NMake Makefiles"
- Optionaly sets flags in order to compile the WPS server and unit tests
- Set include and library directories for libxml2, fastcgi, cppunit

For instance :

Boost_INCLUDE_DIR:PATH=C:/Program Files/boost/boost_1_49_0
Boost_LIBRARY_DIRS:PATH=C:/Program Files/boost/boost_1_49_0/stage/lib
CPPUNIT_INCLUDE_DIR:PATH=E:/libs/cppunit-1.12.1/include
CPPUNIT_LIBRARY_DIRS:PATH=E:/libs/cppunit-1.12.1/lib
FCGI_INCLUDE_DIR:PATH=E:/libs/fcgi-2.4.1-SNAP-0910052249/include
FCGI_LIBRARY_DIRS:PATH=E:/libs/fcgi-2.4.1-SNAP-0910052249/libfcgi/Debug
ICONV_INCLUDE_DIR:PATH=E:/libs/iconv-1.9.2.win32/include
ICONV_LIBRARY_DIRS:PATH=E:/libs/iconv-1.9.2.win32/lib
PQ_INCLUDE_DIR:PATH=C:/Program Files/PostgreSQL/9.1/include
PQ_LIBS_DIR:PATH=C:/Program Files/PostgreSQL/9.1/lib
XML_INCLUDE_DIR:PATH=E:/libs/libxml2-2.7.8.win32/include
XML_LIBRARY_DIRS:PATH=E:/libs/libxml2-2.7.8.win32/lib 

- Now type "nmake" to build the project

QGIS Plugin installation
========================

See README

WPS, FCGI and webserver configuration
=====================================

See README
