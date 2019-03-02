project(libputty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(MINGW)
  add_definitions("-D_WINDOWS")
endif()

set(SRC
  ../../libs/putty/conf.c
  ../../libs/putty/misc.c
  ../../libs/putty/tree234.c
  ../../libs/putty/version.c
  ../../libs/putty/windows/winmisc.c
  ../../libs/putty/windows/winpgntc.c
  ../../libs/putty/windows/winsecur.c
  libputty.c
  libputty.h
  )

include_directories(
  ../../libs/putty
  ../../libs/putty/windows
  .
  )

add_library(
  libputty
  ${SRC}
  )
