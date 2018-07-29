# cmake -DGENERATOR="Visual Studio 15 2017" -P zlib.cmake

string(REPLACE " " "_" GENERATOR_ ${GENERATOR})

set(SRC_DIR_BASE "zlib-1.2.11")
set(SRC_ARC "zlib-1.2.11.tar.xz")
set(SRC_URL "https://zlib.net/zlib-1.2.11.tar.xz")
set(SRC_ARC_HASH_SHA256 4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/donwload/zlib")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/zlib/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/zlib/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/zlib/build_${GENERATOR_}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/zlib_${GENERATOR_}")

if(NOT EXISTS ${SRC_DIR}/README)

  file(DOWNLOAD
	${SRC_URL}
	${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
    SHOW_PROGRESS
    )

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTRACT_DIR}
    )

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )

endif()

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
