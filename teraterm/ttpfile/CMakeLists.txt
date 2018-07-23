
project(ttpfile)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../common/dlglib.cpp
  ../common/dlglib.h
  ../common/ttlib.c
  ../common/ttlib.h
  bplus.c
  bplus.h
  ftlib.c
  ftlib.h
  kermit.c
  kermit.h
  quickvan.c
  quickvan.h
  ttfile.c
  xmodem.c
  xmodem.h
  ymodem.c
  ymodem.h
  zmodem.c
  zmodem.h
  file_res.h
  ttpfile-version.rc
  ttpfile.rc
  ttpfile.def
  )

include_directories(
  ../common
  )

add_library(
  ttpfile SHARED
  ${SRC}
  )

target_link_libraries(
  ttpfile
  ttpcmn
  )
