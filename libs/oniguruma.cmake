
#set(GENERATOR "Visual Studio 15 2017")
string(REPLACE " " "_" GENERATOR_ ${GENERATOR})

set(SRC_DIR_BASE "onig-6.8.2")
set(SRC_ARC "onig-6.8.2.tar.gz")
set(SRC_URL "https://github.com/kkos/oniguruma/releases/download/v6.8.2/onig-6.8.2.tar.gz")
set(SRC_ARC_HASH_SHA1 4bd58a64fcff233118dcdf6d1ad9607c67bdb878)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/donwload/oniguruma")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src/${SRC_DIR_BASE}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/build_${GENERATOR_}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/oniguruma_${GENERATOR_}")

if(NOT EXISTS ${SRC_DIR}/README.md)

  file(DOWNLOAD
	${SRC_URL}
	${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA1=${SRC_ARC_HASH_SHA1}
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
  -DBUILD_SHARED_LIBS=OFF
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake generate fail ${rv}")
endif()

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

file(MAKE_DIRECTORY "${BUILD_DIR}_debug")

execute_process(
  COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${GENERATOR}
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}_debug
  -DBUILD_SHARED_LIBS=OFF
  WORKING_DIRECTORY ${BUILD_DIR}_debug
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake generate fail ${rv}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config debug
  WORKING_DIRECTORY ${BUILD_DIR}_debug
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config debug --target install
  WORKING_DIRECTORY ${BUILD_DIR}_debug
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake install fail ${rv}")
endif()
