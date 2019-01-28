project(teraterm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

if (MSVC)
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG            ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE          ${CMAKE_C_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_CXX_FLAGS_RELEASE})
endif ()


if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()
if(LINK_UNICOWS)
#  set(UNICOWS_LIB "${CMAKE_SOURCE_DIR}/libs/unicows_mingw/liblibunicows.a")
  set(UNICOWS_LIB "${CMAKE_SOURCE_DIR}/libs/libunicows-1.1.2-msvc6/unicows.lib")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/tt_res.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/ttftypes.h
  ../common/ttplugin.h
  ../common/tt-version.h
  ../common/teraterm.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/dlglib.c
  ../common/dlglib_cpp.cpp
  ../common/dlglib.h
  ../common/dlglib_tmpl.cpp
  ../common/compat_win.cpp
  ../common/compat_win.h
  ../common/tmfc.cpp
  ../common/tmfc.h
  ../common/tmfc_frame.cpp
  ../common/i18n.c
  ../common/i18n.h
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

set(TTDLG_SRC
  ../ttpdlg/dlg_res.h
  ../ttpdlg/ttdlg.c
  ../ttpdlg/ttpdlg.rc
  )

source_group(
  "ttdlg"
  FILES
  ${TTDLG_SRC}
  )

set(SRC
  addsetting.cpp
  addsetting.h
  buffer.c
  buffer.h
  clipboar.c
  clipboar.h
  commlib.c
  commlib.h
  dnddlg.cpp
  dnddlg.h
  dnddlg.rc
  filesys.cpp
  filesys.h
  ftdlg.cpp
  ftdlg.h
  keyboard.c
  keyboard.h
  prnabort.cpp
  prnabort.h
  protodlg.cpp
  protodlg.h
  sizetip.c
  sizetip.h
  teklib.c
  teklib.h
  tekwin.cpp
  tekwin.h
  telnet.c
  telnet.h
  teraapp.h
  teraprn.cpp
  teraprn.h
  teraterm.cpp
  ttdde.c
  ttdde.h
  ttdialog.c
  ttdialog.h
  ttermpro.rc
  ttfileio.c
  ttfileio.h
  ttime.c
  ttime.h
  ttplug.c
  ttplug.h
  ttsetup.c
  ttsetup.h
  ttwinman.c
  ttwinman.h
  ttwsk.c
  ttwsk.h
  vtdisp.c
  vtdisp.h
  vtterm.c
  vtterm.h
  vtwin.cpp
  vtwin.h
  winjump.c
  winjump.h
  WSAAsyncGetAddrInfo.c
  WSAAsyncGetAddrInfo.h
  ttutil.c
  ttutil.h
  #
  teraterm.manifest
  #
  ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h
  #
  ${COMMON_SRC}
  ${TTDLG_SRC}
  )

include_directories(
  ../common
  ../ttpfile
  ../ttpdlg
  ${SFMT_INCLUDE_DIRS}
  ${ONIGURUMA_INCLUDE_DIRS}
  .
  ${CMAKE_CURRENT_BINARY_DIR}
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  ${SFMT_LIBRARY_DIRS}
  )

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:imagehlp.dll /DELAYLOAD:user32.dll /DELAYLOAD:shell32.dll")
endif()

add_executable(
  teraterm WIN32
  ${SRC}
  )

set_target_properties(
  teraterm
  PROPERTIES
  OUTPUT_NAME "ttermpro"
  )

target_link_libraries(
  teraterm
  ${UNICOWS_LIB}
  ttpcmn
  ttpfile
  ttpset
  ttptek
  #  ttpdlg
  optimized onig
  debug onigd
  #
  gdi32
  comctl32
  ws2_32
  imm32
  imagehlp
  delayimp
  oleaut32
  uuid
  )

add_dependencies(
  teraterm
  ttpcmn
  ttpfile
  ttpset
  ttptek
  )

configure_file(
  svnversion.h.in
  svnversion.h @ONLY)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ttpdlg/svnversion.h")
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/../ttpdlg/svnversion.h")
endif()
