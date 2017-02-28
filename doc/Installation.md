Installation
============

Dependencies
------------

Tempus depends on the following third party libraries:

* [boost](http://www.boost.org/): timer, system, unit_test_framework, thread and program_options. Version 1.49 works OK on a recent Ubuntu box, consider using version 1.53 for Windows compilation with Visual Studio 2008
* [PostgreSQL](http://www.postgresql.org/)
* [PostGIS](http://postgis.refrections.net/) 2.x
* [LibXML2](http://www.xmlsoft.org/) for the WPS server
* [iconv](http://www.gnu.org/software/libiconv/)
* [shapelib](http://shapelib.maptools.org/) for osm2shp
* [protobuf](https://developers.google.com/protocol-buffers/) for osm2tempus
* [FastCGI](http://www.fastcgi.com/) for the WPS server
* [QGIS](http://www.qgis.org/) 2.x
* [PyQT](http://www.riverbankcomputing.co.uk/software/pyqt/intro) for QGIS plugins

On Ubuntu
---------

On Ubuntu 16.04, dependencies correspond to the following packages:

```
nginx postgresql postgresql-server-dev-9.5 postgis libpq-dev libgeos-dev g++ libboost-all-dev
libfcgi-dev libshp-dev libxml2-dev cmake pyqt4-dev-tools libtool libproj-dev libgdal-dev libprotobuf-dev
```

Compilation
-----------

In the root directory, create a build directory and enter it. Then use a CMake configuration interface (ccmake for instance or cmake-gui on Windows)

```
ccmake ..
```
Set the compilation options as needed (see below). Then, launch the compilation by `make`.

You can launch `make install` (usually as root, with 'sudo' under Ubuntu) to install everything.

Another step is needed if you choose to build QGIS plugins and want them to be installed for the current user. From the install directory, run `sh install-qgis-plugins.sh` on Linux and `install_qgis_plugins.bat` on Windows.

Have a look at [this script](../script/install-tempus-ubuntu.sh) for the installation process on an Ubuntu system.

Build options (CMake options)
-----------------------------

| Option name          | Default      | Description                                                           |
|----------------------|--------------|-----------------------------------------------------------------------|
|CMAKE_INSTALL_PREFIX  | /usr/local   | Specifies where Tempus will be installed when make install is invoked |
|CMAKE_BUILD_TYPE      |              | Switches between a Release build, specially crafted for speed efficiency and a Debug build, specifically crafted for development. |
|BUILD_CORE            | ON           | Whether to build the core or not (needed) |
|BUILD_INTRALYS_PLUGIN | ON           | Whether to build the Intralys QGIS plugin |
|BUILD_QGIS_PLUGIN     | ON           | Whether to build the routing QGIS plugin or not |
|BUILD_TEMPUS_PLUGINS  | ON           | Whether to build the sample routing plugins or not |
|BUILD_TESTS           | ON           | Set to ON to compile unit tests |
|BUILD_WPS             | ON           | Set to ON to compile the WPS server. |

Note: Tempus uses a lot of C++ templates. It means a 'Debug' build will turn off any optimization (`-O0` for gcc) and will run very slowly compared to a version compiled in 'Release' or
'RelWithDebInfo' mode, where optimizations (`-O2` or `-O3`) occur. Optimized versions can be something like *20x faster* than the debug versions !

On Windows (osbolete)
---------------------

A [Windows installer](https://github.com/Ifsttar/Tempus/releases/download/v1.2.1/tempus-1.2.1-win32-vc9.exe) is provided. It will handle the installation of every Tempus components, including PostgreSQL/PostGIS and QGIS if needed.

![Image](images/windows_install_animation.gif?raw=true)

([Source images](images/windows_install_steps.zip?raw=true) of the animation)

If you want to compile Tempus from sources and setup a build environment on Windows, refer to this [README](README_WIN32.md) file.

Test database creation
----------------------
You can test the installation by running some unit tests. They need a test database. The needed steps to build it are :

* Create a postgreSQL database `tempus_test_db`
* Install PostGIS on it (`CREATE EXTENSION postgis;`)
* Unzip `data/tempus_test_db/tempus_test_db.sql.zip`
* Populate it with the Tempus schema and test data: `psql tempus_test_db < data/tempus_test_db/tempus_test_db.sql`
* execute each `patch.n.sql` from `data/tempus_test_db` folders.

Installation of Python modules
------------------------------

There are three Python modules for Tempus:

* The Tempus data loader
* A WPS Python module that is used to communicate with a Tempus server (needed by the QGIS plugin)
* A Python API

Go to the `python` folder and in each subfolder (tempusloader, wpstempus and pytempus), type:
`python setup.py install`.

You will need administrator privileges (or you could also install these modules in your Python virtualenv).

QGIS plugin installation
------------------------

Just type `nmake install` on Windows.

On Linux, type `make install` as root and you will also need to launch `sh /usr/local/share/tempus/install-qgis-plugins.sh` after the installation as user.

It should install the plugin and all required dependencies to your local qgis plugin directory.

When QGIS is launched, go to the "Extension management" dialog, look for "IFSTTAR Routing plugin" and enable it.

Server installation
-------------------

The idea is to use the standalone mode of the WPS server and an httpd server (like nginx or Apache2) that just "passes" execution to this executable.

You have to launch the "tempus_wps" executable from a command line, for example like this:

```
hme@socompa:~/build$ tempus_wps --data ../data -c ./lib -p 9000 -l sample_multi_plugin -l sample_pt_plugin -l sample_road_plugin -t 4
```

Options used:

* `-c ./lib`: will start the server in the given directory, where plugins (.so files) are located
* `-l plugin`: will load the given plugin
* `-t 4`: number of threads to use for parallel request handling
* The option `-d "options"` would also allow to specify how to connect to the underlying database (default to `"dbname=tempus_test_db"`).

This will create a listening socket on port 9000 and start a WPS server. This way, you can see debugging outputs.

You can also launch it as a system daemon. Refer to this [file](../script/tempus.service) to install it with systemd.

Apache configuration
--------------------

Download apache2 and mod_fastcgi or install package libapache2-mod-fastcgi (Ubuntu).

After installation, within the Apache2 configuration directory (/etc/apache2 under Linux), check that the fastcgi module is correctly loaded. A link to `mods-available/mod_fastcgi.conf` must exist in `mods-enabled`.

Then this file must have the following first line :

```
FastCgiExternalServer /var/www/wps -host 127.0.0.1:9000
```

Restart the Apache server. You can now test the WPS installation with a browser pointing to http://127.0.0.1/wps?service=wps&version=1.0.0&request=GetCapabilities

nginx configuration
-------------------

On Windows, nginx can be downloaded from http://nginx.org/en/download.html. Just unzip it wherever you like. On Ubuntu you can install the package nginx.

You can now configure nginx, by adding the following lines inside the `server {...}` configuration part of nginx.conf (`/etc/nginx/sites-available/default` on Ubuntu)

```
location /wps {
        include fastcgi_params;
        fastcgi_pass 127.0.0.1:9000;
}
```

nginx has normally to be launched only once.

Unit tests
----------

You can test your installation by running some unit tests. Within the build directory, you can run ctest in order to test the core components. Example of output:

```
~/src/Tempus/build$ ctest
Test project /home/hme/src/tempus/build
    Start 1: test_core
1/2 Test #1: test_core ........................   Passed    9.82 sec
    Start 2: test_pt
2/2 Test #2: test_pt ..........................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   9.83 sec
```

Other unit tests are provided. They are used to test the WPS and Python parts, the loader and plugins.
A WPS server must be running with all sample plugins loaded on 127.0.0.1/wps in order to launch them.
In order to test the WPS and Python parts, you can run the Python script `test_wps.py` (a WPS with all the sample plugins loaded must be available on 127.0.0.1/wps).

Provided tests are :

* `test_wps.py` : tests communication with a WPS server
* `test_loader.py` (in python/tempusloader) : tests the loader script. *Note:* must have access to a database named `tempus_unit_test`
* `test_plugins.py` : launch unit tests for plugins that have such tests.
* `test_perfs.py` : outputs some computation timing, in order to test performances.

