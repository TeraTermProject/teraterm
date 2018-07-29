project(ttpdlg)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

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

set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /i ${CMAKE_CURRENT_SOURCE_DIR}/../common")

set(SRC
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/teraterm.ico
  dlg_res.h
  ttdlg.c
  ttdlg.h
  ttpdlg.def
  ttpdlg.rc
  ttpdlg-version.rc
  ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${CMAKE_CURRENT_BINARY_DIR}
  ${ONIGURUMA_INCLUDE_DIRS}
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  )

add_library(
  ttpdlg SHARED
  ${SRC}
  )

target_link_libraries(
  ttpdlg
  ttpcmn
  optimized onig.lib
  debug onigd.lib
  )

add_dependencies(
  ttpdlg
  ttpcmn
  )

## svnversion.h
if(EXISTS ${CMAKE_SOURCE_DIR}/.svn)
  INCLUDE(FindSubversion)
endif()
if(Subversion_FOUND)
  Subversion_WC_INFO(${CMAKE_SOURCE_DIR} TT)
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h "#define SVNVERSION ${TT_WC_REVISION}\n")
  message("rev.${TT_WC_REVISION}")
else()
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h "#undef SVNVERSION\n")
endif()
