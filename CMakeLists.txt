project( tempus )
cmake_minimum_required(VERSION 2.6)
enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# FIXME use installation directory on install
#set(TEMPUS_DATA_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data")
if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files(x86)/PostgreSQL/9.3/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files (x86)/PostgreSQL/9.3/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS libpq )
    set (Boost_USE_STATIC_LIBS ON)
	set( LIBTEMPUS tempus )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS pq dl boost_program_options )
	set (LIBTEMPUS tempus )
endif (WIN32)

find_package(Boost REQUIRED COMPONENTS timer system unit_test_framework thread)

include_directories( ${PQ_INCLUDE_DIR} ${Boost_INCLUDE_DIR} )
link_directories( ${PQ_LIBS_DIR} ${Boost_LIBRARY_DIRS} )

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
    set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -Wno-long-long -pedantic -Wpointer-arith -Wcast-align -Wcast-qual -Woverloaded-virtual -Wformat=2 -Winit-self -Wmissing-include-dirs -Wwrite-strings -Wno-error=undef -Wshadow -Wno-error=format" CACHE STRING "compile flags" FORCE)#-Wfloat-equal -Wconversion
    add_definitions( -Wl,-E )
elseif(CMAKE_CXX_COMPILER MATCHES ".*clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Werror " CACHE STRING "compile flags" FORCE)
endif()

set(CPACK_PACKAGE_NAME "tempus")
set(CPACK_PACKAGE_VENDOR "IFSTTAR")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "tempus - manipulate transport graphs")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "tempus-${CPACK_PACKAGE_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Applications")
if (WIN32)
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  ; **** SET TEMPUS_DATA_DIRECTORY IN REGISTRY ***
   !include 'winmessages.nsh'
   WriteRegExpandStr HKLM 'SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment' TEMPUS_DATA_DIRECTORY $INSTDIR\\\\data
   SendMessage \\\${HWND_BROADCAST} \\\${WM_WININICHANGE} 0 'STR:Environment' /TIMEOUT=5000
  ; **** INSTALL POSTGRESQL ***
  MessageBox MB_YESNO 'Install PostgreSQL 9.3 database server ?' /SD IDYES IDNO endPostgres
    NSISdl::download http://get.enterprisedb.com/postgresql/postgresql-9.3.1-1-windows.exe $INSTDIR\\\\postgresql-9.3.1-1-windows.exe
    ExecWait postgresql-9.3.1-1-windows.exe
    Goto endPostgres
  endPostgres:
  ; *** INSTALL NGINX ***
  MessageBox MB_YESNO 'Install nginx web srever ?' /SD IDYES IDNO endNginx
    ExecWait nginx-1.5.6.exe
    ExecWait `..\\\\nginx\\\\nginx_service.exe install`
    ExecWait `net start nginx`
    Goto endNginx   
  endNginx:
  ; CREATE THE TEST DB ----------------------------------
  ExecWait `..\\\\PostgreSQL\\\\9.3\\\\bin\\\\psql.exe -U postgres -f data\\\\tempus_test_db.sql`     
  ; *** INSTALL AND START WPS SERVICE ***
  ExecWait `wps_service.exe install`
  ExecWait `net start wps`
"
)
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  DeleteRegValue HKLM 'SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment' TEMPUS_DATA_DIRECTORY
  ExecWait `net stop wps`
  ExecWait `\\\$INSTDIR\\\\wps_service.exe uninstall`
  Delete \\\$INSTDIR\\\\wps_service.err.log
  Delete \\\$INSTDIR\\\\wps_service.out.log
  Delete \\\$INSTDIR\\\\wps_service.wrapper.log
  RMDir \\\$INSTDIR
")
#Push `TEMPUS_DATA_DIRECTORY`
#Push $INSTDIR\\\\data
#Call ConditionalAddToRegisty
#set(CPACK_NSIS_MODIFY_PATH ON)
endif()
include(CPack)

if (WIN32)
   install(FILES nginx-1.5.6.exe wps_service.exe wps_service.xml DESTINATION .)
endif()

if (WIN32)
  set(TEMPUS_INSTALL_DATA_DIRECTORY .)
else()
  set(TEMPUS_INSTALL_DATA_DIRECTORY "share/${CMAKE_PROJECT_NAME}")
endif()
install(DIRECTORY data DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY})

add_subdirectory(src)
add_subdirectory(doc)

 #cmake -DBUILD_TESTS=ON -DLIBXML2_INCLUDE_DIR="C:\Program Files (x86)\libxml2-2.7.8.win32\include" -DLIBXML2_LIBRARIES="C:\Program Files (x86)\libxml2-2.7.8.win32\lib\libxml2.lib" -DPQ_INCLUDE_DIR="C:/Program Files (x86)/PostgreSQL/9.3/include" -DPQ_LIBS_DIR="C:/Program Files (x86)/PostgreSQL/9.3/lib" -DBUILD_WPS=ON ..
 #set path=Path=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools;C:\Windows\Microsoft.NET\Framework\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v3.5;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\VCPackages;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin\NETFX 4.0 Tools;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;C:\Python33\Lib\site-packages\PyQt4;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files (x86)\CMake 2.8\bin;C:\Program Files (x86)\Git\cmd;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;C:\Program Files (x86)\libxml2-2.7.8.win32\bin;C:\Users\oslandia\iconv-1.9.2.win32\bin;C:\Program Files (x86)\PostgreSQL\9.3\bin;C:\Users\oslandia\fcgi-2.4.1\libfcgi\Release;C:\Users\oslandia;C:\Users\oslandia\tempus\build\lib\Debug
 
 
