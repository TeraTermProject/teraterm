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
  ../common/dlglib.h
  ../common/dlglib.c
  ../common/dlglib_cpp.cpp
  ../common/dlglib_tmpl.cpp
  ../common/win16api.h
  ../common/win16api.c
  ../common/codemap.h
  ../common/compat_w95.h
  ../common/compat_win.h
  ../common/compat_win.cpp
  ../common/i18n.h
  ../common/dllutil.cpp
  ../common/dllutil.h
  ../common/codeconv.h
  ../common/codeconv.cpp
  ../common/tipwin.h
  ../common/tipwin.cpp
  #
  ../teraterm/unisym2decsp.map
  ../teraterm/uni2sjis.map
  ../ttpcmn/sjis2uni.map
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
  imagehlp
  delayimp
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

# copy .lng files
if(${CMAKE_GENERATOR} MATCHES "Visual Studio 15 2017")
  add_custom_target(
    copy_lang ALL
    DEPENDS
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Traditional Chinese.lng"
    )
  add_custom_command(
    OUTPUT
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Default.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/English.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/French.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/German.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Japanese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Korean.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Russian.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Simplified Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/installer/release/lang/Traditional Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$(Configuration)/lang/Traditional Chinese.lng"
    )
endif()
