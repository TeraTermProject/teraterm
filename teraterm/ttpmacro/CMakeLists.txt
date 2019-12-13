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
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/tmfc.cpp
  ../common/tmfc.h
  ../common/tmfc_frame.cpp
  ../common/dlglib.h
  ../common/dlglib.c
  ../common/dlglib_tmpl.cpp
  ../common/dlglib_cpp.cpp
  ../common/compat_w95.h
  ../common/compat_win.h
  ../common/compat_win.cpp
  ../common/win16api.h
  ../common/win16api.c
  ../common/codeconv.h
  ../common/codeconv.cpp
  ../common/dllutil.h
  ../common/dllutil.cpp
  ../common/layer_for_unicode.h
  ../common/layer_for_unicode.cpp
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
  macrodlgbase.h
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
  ttmacro_manifest.rc
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

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:iphlpapi.dll /DELAYLOAD:user32.dll /MANIFEST:NO")
endif()

add_executable(
  ttpmacro WIN32
  ${SRC}
  )

target_link_libraries(
  ttpmacro
  ttpcmn
#  optimized LIBCMT.lib
#  debug LIBCMTD.lib
  optimized onig
  debug onigd
  optimized sfmt
  debug sfmtd
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

install(
  TARGETS ttpmacro
  DESTINATION .
  )
