project(ttpmacro)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/teraterm.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/tt-version.h
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/compat_w95.h
  ../common/win16api.h
  ../common/win16api.c
  ../common/codeconv.h
  ../common/codeconv.cpp
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  ttm_res.h
  errdlg.cpp
  errdlg.h
  inpdlg.cpp
  inpdlg.h
  ListDlg.cpp
  ListDlg.h
  msgdlg.cpp
  msgdlg.h
  statdlg.cpp
  statdlg.h
  stdafx.h
  ttl.c
  ttl.h
  ttm_res.h
  ttmacro.cpp
  ttmacro.h
  ttmbuff.c
  ttmbuff.h
  ttmdde.c
  ttmdde.h
  ttmdef.h
  ttmdlg.cpp
  ttmdlg.h
  ttmenc.c
  ttmenc.h
  ttmlib.c
  ttmlib.h
  ttmmain.cpp
  ttmmain.h
  ttmmsg.h
  ttmonig.h
  ttmparse.c
  ttmparse.h
  wait4all.c
  wait4all.h

  ttpmacro.rc
  ttm-version.rc
  fileread.h
  fileread.cpp
  ${COMMON_SRC}
  )

include_directories(
  ${ONIGURUMA_INCLUDE_DIRS}
  ${SFMT_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  ${SFMT_LIBRARY_DIRS}
  )

set(CMAKE_MFC_FLAG 1)

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:nafxcw.lib /NODEFAULTLIB:LIBCMT.lib")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:nafxcwd.lib /NODEFAULTLIB:LIBCMTD.lib")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:iphlpapi.dll /DELAYLOAD:user32.dll")
endif()

add_executable(
  ttpmacro WIN32
  ${SRC}
  )

target_link_libraries(
  ttpmacro
  ttpcmn
  optimized nafxcw.lib
  debug nafxcwd.lib
  optimized LIBCMT.lib
  debug LIBCMTD.lib
  optimized onig.lib
  debug onigd.lib
  optimized sfmt.lib
  debug sfmtd.lib
  #
  iphlpapi
  delayimp
  ws2_32
  comctl32
  gdi32
  comdlg32
  )

add_dependencies(
  ttpmacro
  ttpcmn
  )
