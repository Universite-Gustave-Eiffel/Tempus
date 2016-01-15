Tempus in Eclipse
=================

Create your Makefile
--------------------

On ubuntu 15.04 :

	INSTALL_PREFIX=~/tempus_installed/libexec/tempus
	MAINTAINER=david.marteau@mappy.com

	CMAKE_FLAGS=-DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/9.4/ \
				-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)


	include lbs.mk


	#------------------------------------------------
	# If you have specific local configuration:
	# create your own Makefile.
	# 
	# Example:
	#
	# BOOST_ROOT=/usr/local/libexec/boost_1_58_0
	# INSTALL_PREFIX=/opt/local/libexec/tempus
	# MAINTAINER=david.marteau@mappy.com
	#
	# CMAKE_FLAGS=-DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/9.4/ \
	#            -DBoost_NO_SYSTEM_PATHS=TRUE \
	#            -DBoost_NO_BOOST_CMAKE=TRUE \
	#            -DBoost_LIBRARY_DIRS:FILEPATH=$(BOOST_ROOT)/lib \
	#            -DBOOST_ROOT:PATHNAME=$(BOOST_ROOT) \
	#            -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)
	#
	#
	# include lbs.mk
	#
	#------------------------------------------------


Create Eclipse project from tempus Makefile
-------------------------------------------

From **File > Import...** :

* In the **Import** window :
> - choose **C/C++ > Existing Code as Makefile project**
* Click next
* In the **New project** window :
> - set a **Project Name**
> - point **Existing Code Location** to your Tempus directory
> - in the **Toolchain for indexer settings**, choose **Linux GCC**
<br/>

Run make targets
----------------

In the **Project explorer**, go to the Tempus root, and double click on **lbs.mk** to open it and make it the active document.

In the **Outline window**, right click on each compile target you plan to use 
and choose **Add make target**.

Targets appear in the thee **Make Target** window.
<br/>

Configure CDT/Codan
-------------------
* In the **Project explorer**, right-click on the Tempus project, and choose the **Properties** item.
* In the Language column, choose **GNU C**.
* Go to **C / C++ General > Paths and symbols**.
* Click on the **Add...** button, and set **Directory** as **/usr/include/libxml2**  
 (you can find the path on your system with command `/usr/include/libxml2`)
<br/>

Build configuration
-------------------
* Go to **Project > Build Configurations > Manage...**
* Make sure you have these two configurations : **Debug** and **Release** ; create them if needed ; close the **Tempus: Manage Configurations** window.
* In the **Project explorer**, right-click on the Tempus project, and choose the **Properties** item.
* Go to **C/C++ Build**
* In the **Configuration** drop down list, select **All Configurations**
* Select the **Builder Settings** tab
> In the **Builder** area, uncheck **Use default build command** (by default, it launches make all) ; for **Build command**, set **make**
* Select the **Behavior** tab
> In the **Workbench Build Behavior** area, ensure these options :  
> [x] **Build (Incremental build)**  : `build`  
> [x] **Clean**  : `clobber configure`
* Close the **Properties for Tempus** window
<br/>

Debugging
---------
* In the **Project explorer**, right-click on the Tempus project, and choose the **Properties** item.
* Go to **C/C++ Build**
* At the right upside corner of the window, click on **Manage Configurations...**
* In the **Configuration** drop down list, select **Debug**
* Select the **Builder Settings** tab, and for **Build command**, set **make DEBUG=1**
* Close the **Properties for Tempus** window
* Go to **Run > Debug Configurations...**
* In the left pane of the window, select **C/C++ Application**, create a new configuration, and do on the new project some settings in the right pane
* **Main** tab :
> - In the field **C/C++ Application:**, set **build_debug/bin/wps**
> - In **Build configuration**, choose **Debug**
> - At the bottom, click on **Select other...**, check **Use configuration specific settings** and choose **Legacy Create Process Create Launcher**
* **Arguments** tab :  
In the field **Program arguments**, enter :<br/>`-c build_debug/lib  -d dbname=tempus -p 7000 -l sample_road_plugin`
* **Environment** tab :  
Add the following variables (adapt to your configuration as set in ...) :

>> **LD_LIBRARY_PATH** : build_debug/lib:/usr/libexec/boost_1_58/lib  
>> **TEMPUS_DATA_DIRECTORY** : data

* **Debugger** tab :
> In the **Debugger Options** area, in the **Use default build command** tab, add the directory `build_debug/lib` for the debug configation.
