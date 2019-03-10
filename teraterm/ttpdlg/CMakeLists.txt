project(ttpdlg)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(MINGW)
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I ${CMAKE_CURRENT_SOURCE_DIR}/../common")
else()
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /i ${CMAKE_CURRENT_SOURCE_DIR}/../common")
endif()

set(COMMON_SRC
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/teraterm.ico
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  dlg_res.h
  ttdlg.c
  ttpdlg.def
  ttpdlg.rc
  ttpdlg-version.rc
  ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h
  ${COMMON_SRC}
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${CMAKE_CURRENT_BINARY_DIR}
  ${ONIGURUMA_INCLUDE_DIRS}
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  )

add_library(
  ttpdlg SHARED
  ${SRC}
  )

if(MINGW)
  set_target_properties(
    ttpdlg
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ttpdlg
  ttpcmn
  optimized onig.lib
  debug onigd.lib
  )

add_dependencies(
  ttpdlg
  ttpcmn
  )

configure_file(
  svnversion.h.in
  svnversion.h @ONLY)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/svnversion.h")
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/svnversion.h")
endif()
