
project(ttpmacro)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  #
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
  )


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/oniguruma/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/SFMT
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )
link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/oniguruma/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/SFMT
  )

set(CMAKE_MFC_FLAG 1)

set(CMAKE_SHARED_LINKER_FLAGS /NODEFAULTLIB:\"LIBCMT.lib,LIBCMTD.lib\")

add_library(
  ttpmacro SHARED
  ${SRC}
  )

target_link_libraries(
  ttpmacro
  ttpcmn
  optimized nafxcw.lib
  debug nafxcwd.lib
  optimized libcmt.lib
  debug libcmtd.lib
  onig_s.lib
  SFMT.lib
  iphlpapi.lib
  )
