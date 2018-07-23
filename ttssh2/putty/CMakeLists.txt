project(libputty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../../libs/putty/CONF.C
  ../../libs/putty/MISC.C
  ../../libs/putty/TREE234.C
  ../../libs/putty/VERSION.C
  ../../libs/putty/WINDOWS/WINMISC.C
  ../../libs/putty/WINDOWS/WINPGNTC.C
  ../../libs/putty/WINDOWS/WINSECUR.C
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
