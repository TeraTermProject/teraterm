project(TTXRecurringCommand)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXRecurringCommand.c
  TTXRecurringCommand.rc
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXRecurringCommand SHARED
  ${SRC}
  )

target_link_libraries(
  TTXRecurringCommand
  ttpcmn
  )
