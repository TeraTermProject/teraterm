project(TTXResizeWin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXResizeWin.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXResizeWin SHARED
  ${SRC}
  )

set_target_properties(
  TTXResizeWin
  PROPERTIES
  PREFIX ""
  )
