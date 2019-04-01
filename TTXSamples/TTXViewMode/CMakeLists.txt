project(TTXViewMode)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  resource.h
  TTXViewMode.c
  TTXViewMode.h
  TTXViewMode.rc
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXViewMode SHARED
  ${SRC}
  )

set_target_properties(
  TTXViewMode
  PROPERTIES
  PREFIX ""
  )
