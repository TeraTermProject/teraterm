project(ttpmacro)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)

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

set(COMMON_SRC
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
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

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:nafxcw.lib /NODEFAULTLIB:LIBCMT.lib")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:nafxcwd.lib /NODEFAULTLIB:LIBCMTD.lib")
if (${MSVC})
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
  iphlpapi.lib
  delayimp.lib
  )

add_dependencies(
  ttpmacro
  ttpcmn
  )
