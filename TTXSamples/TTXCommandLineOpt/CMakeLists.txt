project(TTXCommandLineOpt)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXCommandLineOpt.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXCommandLineOpt SHARED
  ${SRC}
  )

set_target_properties(
  TTXCommandLineOpt
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  TTXCommandLineOpt
  ttpcmn
  )


