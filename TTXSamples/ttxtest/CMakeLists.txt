project(TTXtest)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXtest SHARED
  ttxtest.c
  )

set_target_properties(
  TTXtest
  PROPERTIES
  PREFIX ""
  )
