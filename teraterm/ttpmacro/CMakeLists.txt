project(ttpmacro)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ttpmacro WIN32
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
  ttl.cpp
  ttl.h
  ttl_gui.cpp
  ttl_gui.h
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
  )

target_sources(
  ttpmacro
  PRIVATE
  ../common/teraterm.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/tt-version.h
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/tmfc.h
  ../common/compat_w95.h
  ../common/compat_win.h
  ../common/win16api.h
  ../common/codeconv.h
  ../common/dllutil.h
  ../common/layer_for_unicode.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
  )

target_include_directories(
  ttpmacro
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${ONIGURUMA_INCLUDE_DIRS}
  ${SFMT_INCLUDE_DIRS}
  )

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:iphlpapi.dll /DELAYLOAD:user32.dll /MANIFEST:NO")
endif()

target_link_libraries(
  ttpmacro
  PRIVATE
  common_static
  ttpcmn
  ${ONIGURUMA_LIB}
  ${SFMT_LIB}
  #
  iphlpapi
  delayimp
  ws2_32
  comctl32
  gdi32
  comdlg32
  )

install(
  TARGETS ttpmacro
  DESTINATION .
  )
