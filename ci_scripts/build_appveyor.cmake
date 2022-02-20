option(REMOVE_BUILD_DIR "for clean bulid" OFF)

# create build dir
message(STATUS "BUILD_DIR=${BUILD_DIR}")
if(EXISTS "${BUILD_DIR}")
  if(REMOVE_BUILD_DIR)
    file(REMOVE_RECURSE ${BUILD_DIR})
  else()
    file(REMOVE ${BUILD_DIR}/CMakeCache.txt)
  endif()
endif()
if(NOT EXISTS "${BUILD_DIR}")
  file(MAKE_DIRECTORY ${BUILD_DIR})
endif()


# svn revision
file(MAKE_DIRECTORY ${BUILD_DIR}/teraterm/ttpdlg)
execute_process(
  COMMAND perl ${CMAKE_CURRENT_LIST_DIR}/../buildtools/svnrev/svnrev.pl -v --root "${CMAKE_CURRENT_LIST_DIR}/.." --header ${BUILD_DIR}/teraterm/ttpdlg/svnversion.h --cmake ${BUILD_DIR}/build_config.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../buildtools/svnrev
  )


# variables
include(${BUILD_DIR}/build_config.cmake)
if(NOT DEFINED SVNVERSION)
  set(SVNVERSION "0000")
endif()

set(ZIP_FILE "snapshot-${VERSION}-r${SVNVERSION}-${DATE}_${TIME}-appveyor-${COMPILER_FRIENDLY}.zip")
set(SETUP_FILE "snapshot-${VERSION}-r${SVNVERSION}-${DATE}_${TIME}-appveyor-${COMPILER_FRIENDLY}")
set(SNAPSHOT_DIR "snapshot-${VERSION}-r${SVNVERSION}-${DATE}_${TIME}-appveyor-${COMPILER_FRIENDLY}")

list(APPEND BUILD_OPTIONS "--config" "Release")
list(APPEND GENERATE_OPTIONS "-DSNAPSHOT_DIR=${SNAPSHOT_DIR}" "-DSETUP_ZIP=${ZIP_FILE}" "-DSETUP_EXE=${SETUP_FILE}" "-DSETUP_RELEASE=${RELEASE}")
list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${SNAPSHOT_DIR}")
if(DEFINED CMAKE_C_COMPILER)
  message("CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
endif()
if(DEFINED CMAKE_CXX_COMPILER)
  message("CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif()
if(DEFINED CMAKE_RC_COMPILER)
  message("CMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
endif()


# libs/
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../libs/openssl11_mingw")
  execute_process(
    COMMAND ${CMAKE_COMMAND} "-DCMAKE_GENERATOR=Unix Makefiles" -P ${CMAKE_CURRENT_LIST_DIR}/../libs/buildall.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../libs
    )
endif()


# teraterm
execute_process(
  COMMAND ${CMAKE_COMMAND} "${CMAKE_CURRENT_LIST_DIR}/.." ${GENERATE_OPTIONS}
  WORKING_DIRECTORY ${BUILD_DIR}
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --target install ${BUILD_OPTIONS} ${BUILD_TOOL_OPTIONS}
  WORKING_DIRECTORY ${BUILD_DIR}
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --target zip
  WORKING_DIRECTORY ${BUILD_DIR}
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --target inno_setup
  WORKING_DIRECTORY ${BUILD_DIR}
  )
