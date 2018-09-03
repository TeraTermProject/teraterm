project(teraterm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
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
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  tmfc.h
  tmfc.cpp
  tmfc_frame.cpp
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
  ttutil.c
  ttutil.h
  ${COMMON_SRC}
  )

include_directories(
  ../common
  ../ttpfile
  ../ttpdlg
  ${SFMT_INCLUDE_DIRS}
  .
  )

link_directories(
  ${ONIGURUMA_LIBRARY_DIRS}
  ${SFMT_LIBRARY_DIRS}
  )

set(CMAKE_MFC_FLAG 1)

#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:nafxcw.lib /NODEFAULTLIB:LIBCMT.lib")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:nafxcwd.lib /NODEFAULTLIB:LIBCMTD.lib")
if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:imagehlp.dll /DELAYLOAD:user32.dll /DELAYLOAD:shell32.dll")
endif()

if(MINGW)
  link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../htmlhelp
	)
endif()

add_executable(
  teraterm WIN32
  ${SRC}
  )

target_link_libraries(
  teraterm
  ttpcmn
  ttpdlg
  ttpfile
  ttpset
  ttptek
#  optimized nafxcw.lib
#  debug nafxcwd.lib
#  optimized LIBCMT.lib
#  debug LIBCMTD.lib
  optimized onig.lib
  debug onigd.lib
  Comctl32.lib
  ws2_32.lib
  imm32.lib
  imagehlp.lib
  delayimp.lib
  )

add_dependencies(
  teraterm
  ttpcmn
  ttpdlg
  ttpfile
  ttpset
  ttptek
  )
