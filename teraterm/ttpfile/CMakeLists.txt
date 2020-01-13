project(ttpfile)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/dlglib_cpp.cpp
  ../common/dlglib_tmpl.cpp
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/win16api.h
  ../common/win16api.c
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
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
  ${COMMON_SRC}
  )

include_directories(
  ../common
  )

add_library(
  ttpfile SHARED
  ${SRC}
  )

if(MINGW)
  set_target_properties(
    ttpfile
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DELAYLOAD:user32.dll")
  # target_link_options ...
endif()

target_link_libraries(
  ttpfile
  ttpcmn
  #
  gdi32
  comdlg32
  )

add_dependencies(
  ttpfile
  ttpcmn
  )

install(
  TARGETS ttpfile
  RUNTIME
  DESTINATION .
  )
