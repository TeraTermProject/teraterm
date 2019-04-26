project(teraterm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()
if(ENABLE_DPIAWARENESS)
  add_definitions(-DDPIAWARENESS)
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
  ../common/tmfc.cpp
  ../common/tmfc.h
  ../common/tmfc_frame.cpp
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

set(TTDLG_SRC
  ../ttpdlg/dlg_res.h
  ../ttpdlg/ttdlg.c
  ../ttpdlg/ttdlg.h
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
  teraprn.cpp
  teraprn.h
  teraterm.cpp
  teraterml.h
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
  svnversion.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/../ttpdlg/svnversion.h
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
  ${CMAKE_CURRENT_BINARY_DIR}/../ttpdlg
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
  ttpcmn
  ttpfile
  ttpset
  ttptek
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
  ttpfile
  ttpset
  ttptek
  )

# copy .lng files
if(${MSVC_VERSION} GREATER_EQUAL 1910)
  # Visual Studio 2017以上の時
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

####
configure_file(
  svnversion.h.in
  ../ttpdlg/svnversion.h @ONLY)
