project(TTXRecurringCommand)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

set(COMMON_SRC
  ../../teraterm/common/dlglib.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

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
  ${COMMON_SRC}
  )

target_link_libraries(
  TTXRecurringCommand
  ttpcmn
  #
  gdi32
  )
