project(ttpmenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../teraterm/common/ttlib.c
  ../teraterm/common/ttlib.h
  ../teraterm/common/i18n.c
  ../teraterm/common/i18n.h

  resource.h
  registry.cpp
  ttpmenu.cpp
  winmisc.cpp
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
  )

target_link_libraries(
  ttpmenu
  version
  comctl32
  user32
  gdi32
  comdlg32
  )


