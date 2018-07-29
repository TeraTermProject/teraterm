# cmake -DGENERATOR="Visual Studio 15 2017" -P SFMT.cmake

string(REPLACE " " "_" GENERATOR_ ${GENERATOR})

set(SRC_DIR_BASE "SFMT-src-1.5.1")
set(SRC_ARC "SFMT-1.5.1.zip")
set(SRC_URL "http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/SFMT-src-1.5.1.zip")
set(SRC_ARC_HASH_SHA256 630d1dfa6b690c30472f75fa97ca90ba62f9c13c5add6c264fdac2c1d3a878f4)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/donwload/SFMT")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/SFMT/build_${GENERATOR_}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/SFMT_${GENERATOR_}")

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
  "  FILES SFMT.h SFMT-params.h SFMT-params19937.h\n"
  "  DESTINATION \${CMAKE_INSTALL_PREFIX}/include\n"
  "  )\n"
  )

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

execute_process(
  COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${GENERATOR}
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake generate fail ${rv}")
endif()

########################################

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config release
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config release --target install
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake install fail ${rv}")
endif()

########################################

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config debug
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config debug --target install
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake install fail ${rv}")
endif()
