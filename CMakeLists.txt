project( tempus )
cmake_minimum_required(VERSION 2.6)
enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(src)
add_subdirectory(doc)

if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4" CACHE STRING "compile flags" FORCE)
	endif()
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
 set(CPACK_PACKAGE_VENDOR "ifstar")
 set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "tempus - manipulate transport graphs")
 set(CPACK_PACKAGE_VERSION "1.0.0")
 set(CPACK_PACKAGE_INSTALL_DIRECTORY "tempus_1_0_0")
 set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
 include(CPack)

 #cmake -DBUILD_TESTS=ON -DLIBXML2_INCLUDE_DIR="C:\Program Files (x86)\libxml2-2.7.8.win32\include" -DLIBXML2_LIBRARIES="C:\Program Files (x86)\libxml2-2.7.8.win32\lib\libxml2.lib" ..