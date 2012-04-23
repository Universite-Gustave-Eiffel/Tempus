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
- Run 'cmake-gui ..' (must be in the path)
- Optionaly sets flags in order to compile the WPS server and uni tests
- Set include and library directories for libxml2, fastcgi, cppunit

FCGI and webserver configuration
================================

TODO