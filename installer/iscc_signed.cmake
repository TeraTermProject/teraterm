﻿# cmake -P iscc_signed.cmake

include(${CMAKE_CURRENT_LIST_DIR}/../buildtools/svnrev/build_config.cmake)

#file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed/unzip)
#file(ARCHIVE_EXTRACT
#  INPUT ${CMAKE_CURRENT_LIST_DIR}/Output/portable/teraterm_portable_signed.zip
#  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed/unzip
#)

set(ISCC "${CMAKE_CURRENT_LIST_DIR}/../buildtools/innosetup6/ISCC.exe")

#if(RELEASE)
#  set(SETUP_EXE "teraterm-${VERSION}")
#else()
#  set(SETUP_EXE "teraterm-${VERSION}-${DATE}_${TIME}-${GITVERSION}-$ENV{USERNAME}")
#endif()
set(SRC_DIR "Output/setup_src/teraterm") # teraterm.iss相対
set(SETUP_DIR "${CMAKE_CURRENT_LIST_DIR}/Output/setup")
set(SETUP_EXE "teraterm-unsigned")

file(MAKE_DIRECTORY ${SETUP_DIR})
execute_process(
  COMMAND ${ISCC} /DAppVersion=${VERSION} /DOutputBaseFilename=${SETUP_EXE} /DSrcDir=${SRC_DIR} /O${SETUP_DIR} ${CMAKE_CURRENT_LIST_DIR}/teraterm.iss
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed"
  ENCODING AUTO
  RESULT_VARIABLE rv
)
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()
