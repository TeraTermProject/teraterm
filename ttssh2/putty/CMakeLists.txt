project(libputty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(MINGW)
  add_definitions("-D_WINDOWS")
endif()

set(PUTTY_SRC
  ../../libs/putty/aqsync.c
  ../../libs/putty/callback.c
  ../../libs/putty/errsock.c
  ../../libs/putty/marshal.c
  ../../libs/putty/memory.c
  ../../libs/putty/mpint.c
  ../../libs/putty/sshsh256.c
  ../../libs/putty/tree234.c
  ../../libs/putty/utils.c
  ../../libs/putty/version.c
  ../../libs/putty/windows/wincapi.c
  ../../libs/putty/windows/winhandl.c
  ../../libs/putty/windows/winhsock.c
  ../../libs/putty/windows/winmisc.c
  ../../libs/putty/windows/winnpc.c
  ../../libs/putty/windows/winpgntc.c
  ../../libs/putty/windows/winsecur.c
	  )

source_group(
  "putty"
  FILES
  ${PUTTY_SRC}
  )

set(SRC
  libputty.c
  libputty.h
  putty-import.c
  )

include_directories(
  ../../libs/putty
  ../../libs/putty/windows
  .
  )

add_library(
  libputty
  ${SRC}
  ${PUTTY_SRC}
  )
