project(TTXAlwaysOnTop)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXAlwaysOnTop.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXAlwaysOnTop SHARED
  ${SRC}
  )

set_target_properties(
  TTXAlwaysOnTop
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  TTXAlwaysOnTop
  ttpcmn
  )


