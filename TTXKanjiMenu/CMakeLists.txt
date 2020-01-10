project(ttxkanjimenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  resource.h
  ttxkanjimenu.c
  ttxkanjimenu.rc
  )

include_directories(
  ../teraterm/teraterm
  ../teraterm/common
  ../libs/openssl/inc32
  .
  )

add_library(
  ttxkanjimenu SHARED
  ${SRC}
  )

set_target_properties(
  ttxkanjimenu
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  ttxkanjimenu
  ttpcmn
  #
  ws2_32
  )

install(
  TARGETS ttxkanjimenu
  RUNTIME
  DESTINATION .
  )
