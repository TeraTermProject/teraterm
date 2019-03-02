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

target_link_libraries(
  TTXCommandLineOpt
  ttpcmn
  )


