WIN32 compilation and installation
==================================

- FCGI : http://www.fastcgi.com/dist/fcgi.tar.gz
- Boost http://sourceforge.net/projects/boost/files/boost-binaries/1.53.0
(you will probably have to rename the 'libs-msvc10' (or libs-msvc8) folder to 'lib' for CMake to autodetect libraries)
- Glib (optional, for the C version of osm2shp only) : http://win32builder.gnome.org/packages/3.6/glib-dev_2.34.3-1_win32.zip
- PostgreSQL
- CMake
- OSGeo4W libs :
  - libxml2
  - shapelib
  - iconv
  - pyqt4

FastCGI
=======

- Launch the "Visual Studio 2008|2010 Command Prompt"
- Change the directory to the one that holds FastCGI sources
- Run "nmake /F Makefile.nt"
- It should generates libraries in libfgci\Debug and libfcgi\Release with lots of (hopefully) harmless warnings

Tempus compilation
==================

- Launch a "Visual Studio 2008|2010 Command Prompt"
- Go to the Tempus src/ directory
- Create a 'build' subdirectory
- Enter this directory

- If you also want to compile the QGIS plugin, you should be able to call python, pyuic4 and pyrcc4 from the command line
( If Python and libs have been installed via OsGeo4W, just execute OSGEO4W_INSTALL_DIR\Osgeo4w.bat )
  - Set PYUIC4_PROGRAM to the path of 'pyuic4'
  - Set PYRCC4_PROGRAM to the path of 'pyrcc4'

You can set some environments variables in order to ease the configuration step.
For instance :

```
set FCGI_ROOT=c:\Users\oslandia\fcgi-2.4.1
set BOOST_ROOT=c:\Program Files (x86)\boost_1_53_0
set GLIB_ROOT=c:\Program Files (x86)\glib-dev_2.28.8-1_win32
set PostgreSQL_ROOT=E:\Program Files (x86)\PostgreSQL\9.1
set LIBXML2_ROOT=E:\OSGeo4W
set SHP_ROOT=E:\OSGeo4W
set ICONV_ROOT=E:\OSGeo4W
```

- Run 'cmake-gui ..' (must be in the path)
- When clicking on "Configure", choose "NMake Makefiles"
- Optionaly sets flags in order to compile the WPS server and unit tests
- Set include and library directories for libxml2, fastcgi if needed
- Click "configure" and then "generate"
- Now type "nmake" to build the project

You can type "nmake install" to install

To generate an installer, type "nmake package"
