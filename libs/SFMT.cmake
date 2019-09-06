# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P SFMT.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64 -P SFMT.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P SFMT.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P SFMT.cmake

####
if(("${CMAKE_BUILD_TYPE}" STREQUAL "") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL ""))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
	# multi-configuration
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Release
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P SFMT.cmake
	  )
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Debug
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P SFMT.cmake
	  )
	return()
  elseif("$ENV{MSYSTEM}" MATCHES "MINGW")
	# mingw on msys2
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "Unix Makefiles")
	# mingw
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../mingw.toolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles")
	# VS nmake
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/VSToolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  else()
	# single-configuration
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  endif()
endif()

include(script_support.cmake)

set(SFMT_VERSION "1.5.1")
set(SRC_DIR_BASE "SFMT-src-1.5.1")
set(SRC_ARC "SFMT-1.5.1.zip")
set(SRC_URL "http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/SFMT-src-1.5.1.zip")
set(SRC_ARC_HASH_SHA256 630d1dfa6b690c30472f75fa97ca90ba62f9c13c5add6c264fdac2c1d3a878f4)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/SFMT")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/SFMT_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32"))
  set(BUILD_DIR "${BUILD_DIR}_x64")
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/README.txt)

  file(DOWNLOAD
	${SRC_URL}
	${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
    SHOW_PROGRESS
    )

  file(MAKE_DIRECTORY ${EXTRACT_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/SFMT_version_for_teraterm.h)
  file(WRITE "${SRC_DIR}/SFMT_version_for_teraterm.h"
    "// created by cmake\n"
    "#pragma once\n"
    "#ifndef SFMT_VERSION_H\n"
    "#define SFMT_VERSION_H\n"
    "#define SFMT_VERSION \"${SFMT_VERSION}\"\n"
    "#endif"
    )

endif()
if(NOT EXISTS ${SRC_DIR}/CMakeLists.txt)
  file(WRITE "${SRC_DIR}/CMakeLists.txt"
	"cmake_minimum_required(VERSION 2.4.4)\n"
	"project(SFMT C)\n"
	"\n"
	"if(MSVC)\n"
	"  set(CMAKE_DEBUG_POSTFIX \"d\")\n"
	"endif()\n"
	"\n"
	"add_library(\n"
	"  sfmt STATIC\n"
	"  SFMT.c\n"
	"  )\n"
	"\n"
	"install(\n"
	"  TARGETS sfmt\n"
	"  ARCHIVE DESTINATION \${CMAKE_INSTALL_PREFIX}/lib\n"
	"  )\n"
	"install(\n"
	"  FILES\n"
    "    SFMT.h SFMT-params.h SFMT-params19937.h\n"
    "    SFMT_version_for_teraterm.h\n"
	"  DESTINATION \${CMAKE_INSTALL_PREFIX}/include\n"
	"  )\n"
	)
endif()

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")

  ######################################## multi configuration

  if(NOT "${ARCHITECTURE}" STREQUAL "")
	set(CMAKE_A_OPTION -A ${ARCHITECTURE})
  endif()
  execute_process(
	COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR} ${CMAKE_A_OPTION}
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
	${TOOLCHAINFILE}
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake generate fail ${rv}")
  endif()

  execute_process(
	COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_CONFIGURATION_TYPE} --target install
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

else()
  ######################################## single configuration
  
  execute_process(
	COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake build fail ${rv}")
  endif()
  
  execute_process(
	COMMAND ${CMAKE_COMMAND} --build . --target install
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

endif()
