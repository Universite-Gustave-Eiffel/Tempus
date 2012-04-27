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

Just type 'nmake install' and it should install the plugin and all required dependencies to your local qgis plugin directory

WPS, FCGI and webserver configuration
=====================================

Option 1 - Apache
-----------------

Download mod_fcgid from http://www.apachelounge.com/download/

You will have to setup a directory where FastCGI programs can be called from the Apache webserver.
In the Windows implementation, you have to choose a directory WITHOUT SPACES

Add this to the configuration file (conf/httpd.conf):

---------------------------------------------------
LoadModule fcgid_module modules/mod_fcgid.so

Alias /fcgi-bin/ "E:/fcgi-bin/"
<Directory "E:/fcgi-bin/">
SetHandler fcgid-script
Options +ExecCGI
Order allow,deny
Allow from all
</Directory>
---------------------------------------------------

The WPS executable, Tempus plugin DLLs and DLL dependencies must be copied into the fcgi-bin directory.

Restart the Apache server

You can now test the WPS installation with a browser pointing to http://127.0.0.1/wps?service=wps&version=1.0.0&request=GetCapabilities

The problem with this kind of installation is that it is hard to debug in case of problems. You do not have access to any messages
that could be displayed by the wps executable and Apache must be restarted for every new WPS compilation.

Option 2 - Standalone FCGI with nginx
--------------------------------------

The second option is to use the standalone mode of the WPS server and a light httpd server (like nginx or lighttpd) that
just "passes" execution to this executable.

You have to launch the "wps.exe" executable from a command line, like so:

E:\src\build>wps\wps.exe -c core -p 9000

It will create a listening socket on port 9000 and start wps.exe in the "core" directory
(needed because the core looks for external plugin DLLs which are located in the "core" directory")
This way, you can see debugging outputs.

nginx can be downloaded from http://nginx.org/en/download.html. Just unzip it wherever you like.

You can now configure nginx, by adding the following lines inside the "server {" configuration part of nginx.conf

		location /wps {
                include fastcgi_params;
                fastcgi_pass 127.0.0.1:9000;
        }
		
nginx has normally to be launched only once.
