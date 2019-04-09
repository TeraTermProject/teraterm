# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64 -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake

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
	  -P oniguruma.cmake
	  )
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Debug
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P oniguruma.cmake
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

set(SRC_DIR_BASE "onig-6.9.0")
set(SRC_ARC "onig-6.9.0.tar.gz")
set(SRC_URL "https://github.com/kkos/oniguruma/releases/download/v6.9.0/onig-6.9.0.tar.gz")
set(SRC_ARC_HASH_SHA1 8e3e39e8e92f040939922ddc367a56c12bd4c4c3)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/oniguruma")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/oniguruma_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32"))
  set(BUILD_DIR "${BUILD_DIR}_x64")
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/README.md)

  file(DOWNLOAD
	${SRC_URL}
	${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA1=${SRC_ARC_HASH_SHA1}
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
	-DCMAKE_DEBUG_POSTFIX=d
	-DBUILD_SHARED_LIBS=OFF
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
	-DBUILD_SHARED_LIBS=OFF
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
