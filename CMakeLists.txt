project( tempus )

# the following environment variables must be defined in windows for cmake to find the libraries
# 
# set FCGI_ROOT=c:\Users\oslandia\fcgi-2.4.1
# set BOOST_ROOT=c:\Program Files (x86)\boost_1_54_0
# set GLIB_ROOT=c:\Program Files (x86)\glib-dev_2.28.8-1_win32
# set LIBXML2_ROOT=c:\OSGeo4W
# set PostgreSQL_ROOT=c:\Program Files (x86)\PostgreSQL\9.3
# set SHP_ROOT=c:\OSGeo4W
# set TEMPUS_DATA_DIRECTORY=C:\Program Files (x86)\tempus-1.0.0\data
# set ICONV_ROOT=c:\OSGeo4W

cmake_minimum_required(VERSION 2.6)
enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
if (WIN32)
    set (Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost REQUIRED COMPONENTS timer system unit_test_framework thread program_options date_time)
find_package(PostgreSQL REQUIRED)
include_directories( ${PostgreSQL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} )
link_directories( ${PostgreSQL_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS} )

if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4" CACHE STRING "compile flags" FORCE)
	endif()
	set (CMAKE_EXE_LINKER_FLAGS "/ALLOWBIND")
	
	
	# Disable verbose warnings
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4290" )
	
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

	# Update if necessary
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-long-long -pedantic -Wpointer-arith -Wcast-align -Wcast-qual -Woverloaded-virtual -Wformat=2 -Winit-self -Wmissing-include-dirs -Wwrite-strings -Wno-error=undef -Wshadow -Wno-error=format" CACHE STRING "compile flags" FORCE)#-Wfloat-equal -Wconversion
    add_definitions( -Wl,-E )
elseif(CMAKE_CXX_COMPILER MATCHES ".*clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror " CACHE STRING "compile flags" FORCE)
endif()

set(CPACK_PACKAGE_NAME "tempus")
set(CPACK_PACKAGE_VENDOR "IFSTTAR")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tempus - manipulate transport graphs")
set(CPACK_PACKAGE_VERSION "1.2.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "tempus-${CPACK_PACKAGE_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Applications")
if (WIN32)
#set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  ; **** SET TEMPUS_DATA_DIRECTORY IN REGISTRY ***
   !include 'winmessages.nsh'
   WriteRegExpandStr HKLM 'SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment' TEMPUS_DATA_DIRECTORY $INSTDIR\\\\data
   SendMessage \\\${HWND_BROADCAST} \\\${WM_WININICHANGE} 0 'STR:Environment' /TIMEOUT=5000
   
   ; update startTempus location
   WriteRegExpandStr HKCU 'Software\\\\QGIS\\\\QGIS2\\\\IfsttarTempus' startTempus $INSTDIR\\\\bin\\\\startTempus.bat
   
  ; **** INSTALL POSTGRESQL ***
  MessageBox MB_YESNO 'Install PostgreSQL 9.3 database server ?' /SD IDYES IDNO endPostgres
    NSISdl::download http://get.enterprisedb.com/postgresql/postgresql-9.3.1-1-windows.exe $INSTDIR\\\\postgresql-9.3.1-1-windows.exe
    IfFileExists postgresql-9.3.1-1-windows.exe PostgresExists PostgresMissing
    PostgresExists:
       ExecWait postgresql-9.3.1-1-windows.exe
       Delete postgresql-9.3.1-1-windows.exe
       Goto endPostgres
    PostgresMissing:
       MessageBox MB_OK 'Unable to download PostgreSQL 9.3' 
  endPostgres:
  
  ; **** INSTALL QGIS ***
  MessageBox MB_YESNO 'Install QGIS 2.4 ?' /SD IDYES IDNO endQGIS
    NSISdl::download  http://qgis.org/downloads/QGIS-OSGeo4W-2.4.0-1-Setup-x86.exe $INSTDIR\\\\QGIS-OSGeo4W-2.4.0-1-Setup-x86.exe
    IfFileExists QGIS-OSGeo4W-2.4.0-1-Setup-x86.exe QgisExists QgisMissing
    QgisExists:
       ExecWait QGIS-OSGeo4W-2.4.0-1-Setup-x86.exe
       Delete QGIS-OSGeo4W-2.4.0-1-Setup-x86.exe
       Goto endQGIS
    QgisMissing:
       MessageBox MB_OK 'Unable to download QGIS 2.4' 
  endQGIS:
  
  ; *** INSTALL NGINX ***
  MessageBox MB_YESNO 'Install nginx web server ?' /SD IDYES IDNO endNginx
    ExecWait `nginx-1.5.6.exe`
    ExecWait `..\\\\nginx\\\\nginx_service.exe install`
    ExecWait `net start nginx`
    Goto endNginx   
  endNginx:
 
  ; *** INSTALL MSVC REDISTRIBUTABLES ***
  ExecWait `vcredist_x86.exe /q`
 
  ; *** INSTALL TEST DB ***
  ExecWait `reset_tempus_test_db.bat`

  ; *** INSTALL AND START WPS SERVICE ***
  ;ExecWait `gen_wps_service_xml.bat`
  ;ExecWait `wps_service.exe install`
  ;ExecWait `net start wps`
  
  ; *** INSTALL PLUGINS IN USER .qgis DIRECTORY ***
  ExecWait `install_qgis_plugins.bat`
"
)
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  DeleteRegValue HKLM 'SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment' TEMPUS_DATA_DIRECTORY
  ;ExecWait `net stop wps`
  ;ExecWait `\\\$INSTDIR\\\\wps_service.exe uninstall`
  ;Delete \\\$INSTDIR\\\\wps_service.err.log
  ;Delete \\\$INSTDIR\\\\wps_service.out.log
  ;Delete \\\$INSTDIR\\\\wps_service.wrapper.log
  ;Delete \\\$INSTDIR\\\\wps_service.xml
  Delete \\\$INSTDIR\\\\data\\\\tempus_test_db.sql
  RMDir \\\$INSTDIR
")
#Push `TEMPUS_DATA_DIRECTORY`
#Push $INSTDIR\\\\data
#Call ConditionalAddToRegisty
endif()
include(CPack)

if (WIN32)
   find_file( MSVCR71 NAME msvcr71.dll
     PATHS
     $ENV{OSGEO4W_ROOT}/bin
	 $ENV{MSVCR71_DLL}
   )
   if (MSVCR71)
   else()
        message( FATAL_ERROR "msvcr71.dll not found, define OSGEO4W_ROOT or MSVRC71_DLL in your environment" )
	endif()
   
   file(GLOB intldll ${PostgreSQL_LIBRARY_DIRS}/../bin/libintl*.dll)
   file(GLOB iconvdll ${PostgreSQL_LIBRARY_DIRS}/../bin/*iconv*.dll)
   install(FILES win32/nginx-1.5.6.exe win32/wps_service.exe win32/gen_wps_service_xml.bat win32/vcredist_x86.exe win32/reset_tempus_test_db.bat win32/install_qgis_plugins.bat win32/unzip.vbs DESTINATION .)
   install(FILES 
      ${PostgreSQL_LIBRARY_DIRS}/libpq.dll 
      ${PostgreSQL_LIBRARY_DIRS}/../bin/zlib1.dll 
      ${PostgreSQL_LIBRARY_DIRS}/../bin/libeay32.dll
      ${PostgreSQL_LIBRARY_DIRS}/../bin/ssleay32.dll
      ${intldll}
	  ${iconvdll}
      ${PostgreSQL_LIBRARY_DIRS}/../bin/postgisgui/libglib-2.0-0.dll
      ${PostgreSQL_LIBRARY_DIRS}/../bin/postgisgui/intl.dll
      ${MSVCR71}
      DESTINATION bin)
else()
  install(FILES script/install-qgis-plugins.sh DESTINATION share/tempus)
endif()

if (WIN32)
  set(TEMPUS_INSTALL_DATA_DIRECTORY .)
  set(TEMPUS_INSTALL_DOC_DIRECTORY doc)
  set( PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" )
else()
  set(TEMPUS_INSTALL_DATA_DIRECTORY "share/${CMAKE_PROJECT_NAME}")
  set(TEMPUS_INSTALL_DOC_DIRECTORY "share/doc/${CMAKE_PROJECT_NAME}")
  set( PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" )
endif()
install(DIRECTORY data DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY})

add_subdirectory(src)

set( BUILD_TESTS ON CACHE BOOL "Build unit tests ?" )

if (BUILD_TESTS)
  add_subdirectory( tests )
endif()

set( BUILD_DOC ON CACHE BOOL "Build documentation ?" )

if(BUILD_DOC)
  add_subdirectory( doc )
endif()

if (WIN32)
  install(
    FILES script/startTempus.bat
    DESTINATION bin
    )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/script/startTempus.sh.in ${CMAKE_CURRENT_BINARY_DIR}/script/startTempus.sh )
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/script/startTempus.sh
    DESTINATION bin
    PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
endif()


 #cmake -DBUILD_INTRALYS_PLUGIN=ON -DBUILD_QGIS_PLUGIN=ON -DBUILD_TESTS=ON -DLIBXML2_INCLUDE_DIR="C:\Program Files (x86)\libxml2-2.7.8.win32\include" -DLIBXML2_LIBRARIES="C:\Program Files (x86)\libxml2-2.7.8.win32\lib\libxml2.lib" -DPQ_INCLUDE_DIR="C:/Program Files (x86)/PostgreSQL/9.3/include" -DPQ_LIBS_DIR="C:/Program Files (x86)/PostgreSQL/9.3/lib" -DBUILD_WPS=ON ..
 #set path=Path=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools;C:\Windows\Microsoft.NET\Framework\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v3.5;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\VCPackages;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin\NETFX 4.0 Tools;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;C:\Python27\Lib\site-packages\PyQt4;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files (x86)\CMake 2.8\bin;C:\Program Files (x86)\Git\cmd;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;C:\Program Files (x86)\libxml2-2.7.8.win32\bin;C:\Users\oslandia\iconv-1.9.2.win32\bin;C:\Program Files (x86)\PostgreSQL\9.3\bin;C:\Users\oslandia\fcgi-2.4.1\libfcgi\Release;C:\Users\oslandia;C:\Users\oslandia\tempus\build\lib\Debug
 #set pythonpath=c:\OSGeo4W\apps\Python27\Lib;c:\OSGeo4W\apps\Python27\DLLs

