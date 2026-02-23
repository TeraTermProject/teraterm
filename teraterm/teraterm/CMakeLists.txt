set(PACKAGE_NAME "teraterm")

project(${PACKAGE_NAME})

option(ENABLE_GDIPLUS "Use GDI+ library" ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

set(ENABLE_DEBUG_INFO 1)

add_executable(
  ${PACKAGE_NAME} WIN32
  ${CMAKE_CURRENT_BINARY_DIR}/../common/svnversion.h
  WSAAsyncGetAddrInfo.c
  WSAAsyncGetAddrInfo.h
  broadcast.cpp
  broadcast.h
  buffer.c
  buffer.h
  charset.cpp
  charset.h
  checkeol.cpp
  checkeol.h
  clipboar.c
  clipboar.h
  commlib.c
  commlib.h
  externalsetup.cpp
  externalsetup.h
  filesys.cpp
  filesys.h
  filesys_log.cpp
  filesys_log.rc
  filesys_log_res.h
  filesys_proto.cpp
  filesys_proto.h
  keyboard.c
  keyboard.h
  scp.cpp
  scp.h
  sendmem.cpp
  sendmem.h
  sizetip.c
  sizetip.h
  tekwin.cpp
  tekwin.h
  telnet.c
  telnet.h
  teraprn.cpp
  teraprn.h
  teraterm.cpp
  teraterm_manifest.rc
  teraterml.h
  theme.h
  themefile.cpp
  tslib.h
  tslib.cpp
  tt-version.rc
  ttdde.c
  ttdde.h
  ttdialog.c
  ttdialog.h
  ttermpro.rc
  ttfileio.c
  ttfileio.h
  ttime.c
  ttime.h
  ttplug.cpp
  ttplug.h
  ttsetup.c
  ttsetup.h
  ttwinman.c
  ttwinman.h
  ttwsk.c
  ttwsk.h
  unicode_test.h
  vtdisp.c
  vtdisp.h
  vtdraw.cpp
  vtdraw.h
  vtterm.c
  vtterm.h
  vtwin.cpp
  vtwin.h
  winjump.c
  winjump.h
)

set(SRC_DIALOG
  ../common/tt_res.h
  addsetting.cpp
  addsetting.h
  clipboarddlg.cpp
  clipboarddlg.h
  coding_pp.cpp
  coding_pp.h
  coding_pp.rc
  coding_pp_res.h
  color_sample.cpp
  color_sample.h
  commentdlg.cpp
  commentdlg.h
  debug_pp.cpp
  debug_pp.h
  debug_pp.rc
  debug_pp_res.h
  dnddlg.cpp
  dnddlg.h
  font_pp.cpp
  font_pp.h
  font_pp.rc
  font_pp_res.h
  ftdlg.cpp
  ftdlg.h
  ftdlg_lite.cpp
  ftdlg_lite.h
  general_pp.cpp
  general_pp.h
  keyboard_pp.c
  keyboard_pp.h
  log_pp.cpp
  log_pp.h
  logdlg.cpp
  logdlg.h
  mouse_pp.cpp
  mouse_pp.h
  mouse_pp.rc
  plugin_pp.cpp
  plugin_pp.h
  plugin_pp.rc
  plugin_pp_res.h
  prnabort.cpp
  prnabort.h
  protodlg.cpp
  protodlg.h
  recvfiledlg.cpp
  recvfiledlg.h
  resource.h
  sendfiledlg.cpp
  sendfiledlg.h
  serial_pp.cpp
  serial_pp.h
  setupdirdlg.cpp
  setupdirdlg.h
  tekfont_pp.cpp
  tekfont_pp.h
  tekfont_pp.rc
  tekfont_pp_res.h
  tcpip_pp.cpp
  tcpip_pp.h
  term_pp.cpp
  term_pp.h
  theme_pp.cpp
  theme_pp.h
  themedlg.cpp
  themedlg.h
  themedlg.rc
  themedlg_res.h
  win_pp.cpp
  win_pp.h
  ui_pp.cpp
  ui_pp.h
  ui_pp.rc
  ui_pp_res.h
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ${SRC_DIALOG}
)

source_group(
  "dialog"
  FILES ${SRC_DIALOG})

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/uni2sjis.map
  ../teraterm/unisym2decsp.map
  ../ttpcmn/sjis2uni.map
  uni_combining.map
  unicode_asian_width.tbl
  unicode_combine.tbl
  unicode_emoji.tbl
)

source_group(
  "table_datas"
  REGULAR_EXPRESSION
  "(map|tbl)$"
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../common/tt_res.h
  ../common/ttcommon.h
  ../common/ttddecmnd.h
  ../common/tttypes.h
  ../common/tektypes.h
  ../common/ttplugin.h
  ../common/tt-version.h
  ../common/teraterm.h
  ../common/ttlib.h
  ../common/dlglib.h
  ../common/codemap.h
  ../common/compat_win.h
  ../common/tmfc.h
  ../common/i18n.h
  ../common/dllutil.h
  ../common/codeconv.h
  ../common/tipwin.h
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/common/edithistory.cpp
  ../../teraterm/common/edithistory.h
  ../../teraterm/common/edithistory.rc
  ../../teraterm/common/edithistory_res.h
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../susie_plugin/libsusieplugin.cpp
  ../susie_plugin/libsusieplugin.h
)

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../ttpdlg/aboutdlg.c
  ../ttpdlg/choosefontdlg.c
  ../ttpdlg/dlg_res.h
  ../ttpdlg/generaldlg.c
  ../ttpdlg/hostdlg.c
  ../ttpdlg/keyboarddlg.c
  ../ttpdlg/serialdlg.cpp
  ../ttpdlg/tcpipdlg.c
  ../ttpdlg/termdlg.c
  ../ttpdlg/ttdlg.c
  ../ttpdlg/ttdlg.h
  ../ttpdlg/ttpdlg.rc
  ../ttpdlg/windlg.cpp
  ../ttpdlg/windowdlg.cpp
  )

source_group(
  "ttpdlg"
  REGULAR_EXPRESSION
  "..\/ttpdlg\/")

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../ttpfile/bplus.c
  ../ttpfile/bplus.h
  ../ttpfile/filesys_io.h
  ../ttpfile/filesys_win32.cpp
  ../ttpfile/filesys_win32.h
  ../ttpfile/ftlib.c
  ../ttpfile/ftlib.h
  ../ttpfile/kermit.c
  ../ttpfile/kermit.h
  ../ttpfile/protolog.cpp
  ../ttpfile/protolog.h
  ../ttpfile/quickvan.c
  ../ttpfile/quickvan.h
  ../ttpfile/raw.c
  ../ttpfile/raw.h
  ../ttpfile/xmodem.c
  ../ttpfile/xmodem.h
  ../ttpfile/ymodem.c
  ../ttpfile/ymodem.h
  ../ttpfile/zmodem.c
  ../ttpfile/zmodem.h
)

source_group(
  "ttpfile"
  REGULAR_EXPRESSION
  "..\/ttpfile\/")

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../ttpset/ttset.c
  ../ttpset/ttset.h
  ../ttpset/ttset_i.h
  ../ttpset/ttset_keyboard.c
)

source_group(
  "ttpset"
  REGULAR_EXPRESSION
  "..\/ttpset\/")

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../ttpcmn/ttcmn_dup.cpp
  ../ttpcmn/ttcmn_dup.h
  ../ttpcmn/ttcmn_shared_memory.h
  ../ttpcmn/ttcmn_static.c
  ../ttpcmn/ttcmn_static.h
)

source_group(
  "ttpcmn"
  REGULAR_EXPRESSION
  "..\/ttpcmn\/")

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../ttptek/tekesc.c
  ../ttptek/tekesc.h
  ../ttptek/tttek.c
  ../ttptek/tttek.h
)

source_group(
  "ttptek"
  REGULAR_EXPRESSION
  "..\/ttptek\/")

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
      set(CMAKE_C_STANDARD_LIBRARIES "")
      set(CMAKE_CXX_STANDARD_LIBRARIES "")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
    else()
      target_link_options(
        ${PACKAGE_NAME}
        PRIVATE
        /NODEFAULTLIB
        )
    endif()
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

if (MSVC)
  if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  else()
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      /MANIFEST:NO)
  endif()
endif()

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  OUTPUT_NAME "ttermpro"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  ../teraterm
  ../ttpfile
  ../ttpdlg
  ../ttpcmn
  ../susie_plugin
  ../ttptek
  ${ONIGURUMA_INCLUDE_DIRS}
  ${SFMT_INCLUDE_DIRS}
  )

if(MINGW)
  target_include_directories(
    ${PACKAGE_NAME}
    PRIVATE
    .
    )
endif()

if(MSVC AND ENABLE_DEBUG_INFO)
  if(${CMAKE_VERSION} VERSION_GREATER "3.13")
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      /DEBUG:FULL
      /pdbaltpath:ttermpro.pdb
      /map
      )
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /map")
  endif()
elseif(MINGW)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -Wl,-Map=ttermpro.map,-cref
    )
endif()

if(ENABLE_GDIPLUS)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE "-DENABLE_GDIPLUS=1"
  )
else(ENABLE_GDIPLUS)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE "-DENABLE_GDIPLUS=0"
  )
endif(ENABLE_GDIPLUS)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  cyglib
  ${ONIGURUMA_LIB}
  ${SFMT_LIB}
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    layer_for_unicode
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  advapi32
  comctl32
  comdlg32
  gdi32
  kernel32
  ole32
  oleaut32
  shell32
  user32
  uuid
  ws2_32
  )

if(ENABLE_GDIPLUS)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    gdiplus.lib
  )
endif(ENABLE_GDIPLUS)

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )
if(MSVC AND ENABLE_DEBUG_INFO AND (${CMAKE_VERSION} VERSION_GREATER "3.13"))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    install(
      FILES
      $<TARGET_PDB_FILE:teraterm>
      $<CONFIG>/ttermpro.map
      DESTINATION .
      )
  else()
    # single configuration like "NMake Makefiles"
    install(
      FILES
      $<TARGET_PDB_FILE:teraterm>
      ${CMAKE_BINARY_DIR}/ttermpro.map
      DESTINATION .
      )
  endif()
endif()
