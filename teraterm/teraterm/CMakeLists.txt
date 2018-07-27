project(teraterm)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../common/tt_res.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/ttftypes.h
  ../common/ttplugin.h
  ../common/tt-version.h
  ../common/teraterm.h
  #
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/dlglib.c
  ../common/dlglib.h
  #
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
  )


include_directories(
  ../common
  ../ttpfile
  .
  )
link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/oniguruma/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/SFMT
  )
set(CMAKE_MFC_FLAG 1)
set(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:\"nafxcwd.lib,LIBCMTD.lib\")
add_executable(
  teraterm WIN32
  ${SRC}
  )
target_link_libraries(
  teraterm
  debug nafxcwd.lib
  debug LIBCMTD.lib
  ttpcmn
  ttpdlg
  ttpfile
  ttpmacro
  ttpset
  ttptek
  onig_s.lib
  ws2_32.lib
  imm32.lib
  imagehlp.lib
  )


