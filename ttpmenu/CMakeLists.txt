project(ttpmenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../teraterm/common/ttlib.h
  ../teraterm/common/ttlib.c
  ../teraterm/common/i18n.h
  ../teraterm/common/i18n.c
  ../teraterm/common/codeconv.h
  ../teraterm/common/codeconv.cpp
  ../teraterm/common/compat_win.h
  ../teraterm/common/compat_win.cpp
  ../teraterm/common/dllutil.h
  ../teraterm/common/dllutil.cpp
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  resource.h
  registry.cpp
  ttpmenu.h
  ttpmenu.cpp
  winmisc.cpp
  #
  left.ico
  right.ico
  teraterm.ico
  ttpmenu.rc
  )


include_directories(
  ../teraterm/common
  .
  )

add_executable(
  ttpmenu WIN32
  ${SRC}
  ${COMMON_SRC}
  )

target_link_libraries(
  ttpmenu
  version
  comctl32
  user32
  gdi32
  comdlg32
  )

install(
  TARGETS ttpmenu
  DESTINATION .
  )
