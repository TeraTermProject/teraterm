
if(NOT DEFINED CYGWIN_PATH)
  set(CYGWIN_PATH "C:/cygwin64/bin")
  if (NOT EXISTS ${CYGWIN_PATH})
    set(CYGWIN_PATH "C:/cygwin/bin")
    if (NOT EXISTS ${CYGWIN_PATH})
      message(FATAL_ERROR "Not found cygwin")
    endif()
  endif()
endif()

message("build_cygterm.cmake")
message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message("CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
message("CYGWIN_PATH=${CYGWIN_PATH}")
set(ENV{PATH} ${CYGWIN_PATH})
execute_process(
  COMMAND make all
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../cygterm
  )
