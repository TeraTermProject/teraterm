project(TTXCallSysMenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXCallSysMenu.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXCallSysMenu SHARED
  ${SRC}
  )

set_target_properties(
  TTXCallSysMenu
  PROPERTIES
  PREFIX ""
  )
