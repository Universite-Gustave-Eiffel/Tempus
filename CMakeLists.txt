project( tempus )

cmake_minimum_required(VERSION 2.6)
enable_testing()

set( TEMPUS_VERSION_MAJOR 2 )
set( TEMPUS_VERSION_MINOR 2 )
set( TEMPUS_VERSION_PATCH 0 )

# don't rebuild everything when rules are changed
set(CMAKE_SKIP_RULE_DEPENDENCY true)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# for CMake >= 3.1.0
# set_property(TARGET tgt PROPERTY CXX STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
if (WIN32)
    set (Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost REQUIRED COMPONENTS timer system unit_test_framework thread program_options date_time)
find_package(PostgreSQL REQUIRED)
include_directories( ${PostgreSQL_INCLUDE_DIRS} )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
link_directories( ${PostgreSQL_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS} )

set (CMAKE_C_FLAGS "-fPIC")
set (CMAKE_CXX_FLAGS "-fPIC")

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4" CACHE STRING "compile flags" FORCE)
	endif()
	set (CMAKE_EXE_LINKER_FLAGS "/ALLOWBIND")


	# Disable verbose warnings
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4290 /wd4267 /wd4267" )

elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

  # Update if necessary
  # FIXME add Weffc++ to detect uninitialized members
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic -Wpointer-arith -Wcast-align -Wcast-qual -Woverloaded-virtual -Wformat=2 -Winit-self -Wmissing-include-dirs -Wwrite-strings -Wno-error=undef -Wshadow -Wno-error=format" CACHE STRING "compile flags" FORCE)#-Wfloat-equal -Wconversion
	# auto_ptr is deprecated in C++11
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-deprecated" CACHE STRING "compile flags" FORCE)
    add_definitions( -Wl,-E )
elseif(CMAKE_CXX_COMPILER MATCHES ".*clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++11 -Wno-deprecated" CACHE STRING "compile flags" FORCE)
endif()


if (WIN32)
   install(FILES win32/reset_tempus_test_db.bat win32/install_qgis_plugins.bat win32/unzip.vbs DESTINATION .)
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
