project(teraterm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/tt_res.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/tektypes.h
  ../common/ttftypes.h
  ../common/ttplugin.h
  ../common/tt-version.h
  ../common/teraterm.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/win16api.h
  ../common/win16api.c
  ../common/codemap.h
  ../common/compat_w95.h
  ../common/compat_win.h
  ../common/compat_win.cpp
  ../common/i18n.h
  ../common/dllutil.cpp
  ../common/dllutil.h
  ../ttpcmn/language.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
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
  tt-version.rc
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
  #
  uni2sjis.map
  unisym2decsp.map
  uni_combining.map
  #
  teraterm.manifest
  #
  ${COMMON_SRC}
  )

include_directories(
  ../common
  ../ttpfile
  ${SFMT_INCLUDE_DIRS}
  ${ONIGURUMA_INCLUDE_DIRS}
  .
  ${CMAKE_CURRENT_BINARY_DIR}
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  ${SFMT_LIBRARY_DIRS}
  )

set(CMAKE_MFC_FLAG 1)

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:nafxcw.lib /NODEFAULTLIB:LIBCMT.lib")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:nafxcwd.lib /NODEFAULTLIB:LIBCMTD.lib")
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
  ttpcmn
  ttpdlg
  ttpfile
  ttpset
  ttptek
  optimized nafxcw.lib
  debug nafxcwd.lib
  optimized LIBCMT.lib
  debug LIBCMTD.lib
  optimized onig.lib
  debug onigd.lib
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
  ttpdlg
  ttpfile
  ttpset
  ttptek
  )
