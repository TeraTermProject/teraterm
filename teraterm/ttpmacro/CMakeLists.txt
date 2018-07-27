project(ttpmacro)

if (MSVC)
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG            ${CMAKE_C_FLAGS_DEBUG})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_MINSIZEREL       ${CMAKE_C_FLAGS_MINSIZEREL})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE          ${CMAKE_C_FLAGS_RELEASE})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO   ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_MINSIZEREL     ${CMAKE_CXX_FLAGS_MINSIZEREL})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_CXX_FLAGS_RELEASE})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif ()

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

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/NODEFAULTLIB:nafxcw.lib /NODEFAULTLIB:LIBCMT.lib")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/NODEFAULTLIB:nafxcwd.lib /NODEFAULTLIB:LIBCMTD.lib")
set(CMAKE_EXE_LINKER_FLAGS "/DELAYLOAD:iphlpapi.dll /DELAYLOAD:user32.dll")

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
  optimized onig_s.lib
  debug onig_sd.lib
  optimized SFMT.lib
  debug SFMTd.lib
  iphlpapi.lib
  )

add_dependencies(
  ttpmacro
  ttpcmn
  )
