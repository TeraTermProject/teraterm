# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P zlib.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P zlib.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P zlib.cmake

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
	  -P zlib.cmake
	  )
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Debug
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P zlib.cmake
	  )
	return()
  elseif(("$ENV{MSYSTEM}" MATCHES "MINGW") OR ("${CMAKE_COMMAND}" MATCHES "mingw"))
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

set(SRC_DIR_BASE "zlib-1.2.11")
set(SRC_ARC "zlib-1.2.11.tar.xz")
set(SRC_URL "https://zlib.net/zlib-1.2.11.tar.xz")
set(SRC_ARC_HASH_SHA256 4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/zlib")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/zlib/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/zlib/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/zlib/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/zlib_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32") OR ("${CMAKE_COMMAND}" MATCHES "mingw64"))
  set(BUILD_DIR "${BUILD_DIR}_x64")
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/README)

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
