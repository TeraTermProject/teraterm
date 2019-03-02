project(TTXAlwaysOnTop)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

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

target_link_libraries(
  TTXAlwaysOnTop
  ttpcmn
  )


