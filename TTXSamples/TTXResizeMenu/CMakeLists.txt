project(TTXResizeMenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXResizeMenu.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXResizeMenu SHARED
  ${SRC}
  )

set_target_properties(
  TTXResizeMenu
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  TTXResizeMenu
  ttpcmn
  )


