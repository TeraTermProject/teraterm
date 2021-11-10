
if(NOT DEFINED CYGWIN_PATH)
  set(CYGWIN_PATH "C:/cygwin64/bin")
  if (NOT EXISTS ${CYGWIN_PATH})
    set(CYGWIN_PATH "C:/cygwin/bin")
    if (NOT EXISTS ${CYGWIN_PATH})
      message(FATAL_ERROR "Not found cygwin")
    endif()
  endif()
endif()
if (NOT EXISTS ${CYGWIN_PATH}/gcc.exe)
  # package gcc-core
  message("CYGWIN_PATH=${CYGWIN_PATH}")
  message(FATAL_ERROR "Not found cygwin gcc")
endif()
if (NOT EXISTS ${CYGWIN_PATH}/g++.exe)
  # package gpp
  message(FATAL_ERROR "Not found cygwin g++")
endif()

# package mingw64-x86_64-gcc-core

message("build_cygterm.cmake")
message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message("CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
message("CYGWIN_PATH=${CYGWIN_PATH}")
set(ENV{PATH} ${CYGWIN_PATH})
execute_process(
  COMMAND make all
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cygterm build error")
endif()
