project(teraterm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)

set(ENABLE_DEBUG_INFO 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  teraterm WIN32
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
  sendmem.cpp
  sendmem.h
  #
  uni_combining.map
  #
  ttermpro.rc
  tt-version.rc
  teraterm_manifest.rc
  #
  svnversion.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/../ttpdlg/svnversion.h
  #
  unicode_test.h
  unicode.h
  unicode.cpp
  unicode_asian_width.tbl
  unicode_combine.tbl
  unicode_emoji.tbl
  #
  ../ttpmacro/fileread.h
  ../ttpmacro/fileread.cpp
  ftdlg_lite.h
  ftdlg_lite.cpp
  clipboarddlg.h
  clipboarddlg.cpp
  debug_pp.cpp
  debug_pp.h
  debug_pp.rc
  )

target_sources(
  teraterm
  PRIVATE
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
  ../common/layer_for_unicode.h
  ../common/layer_for_unicode.cpp
  #
  ../teraterm/unisym2decsp.map
  ../teraterm/uni2sjis.map
  ../ttpcmn/sjis2uni.map
  )

source_group(
  "table_datas"
  REGULAR_EXPRESSION
  "(map|tbl)$"
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
  )

target_sources(
  teraterm
  PRIVATE
  ../ttpdlg/dlg_res.h
  ../ttpdlg/ttdlg.c
  ../ttpdlg/ttdlg.h
  ../ttpdlg/ttpdlg.rc
  )

source_group(
  "ttpdlg"
  REGULAR_EXPRESSION
  "..\/ttpdlg\/")

if (MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:user32.dll /DELAYLOAD:shell32.dll /MANIFEST:NO")
  if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    # 32bit
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:imagehlp.dll")
  endif()
endif()

set_target_properties(
  teraterm
  PROPERTIES
  OUTPUT_NAME "ttermpro"
  )

target_include_directories(
  teraterm
  PRIVATE
  ../common
  ../ttpfile
  ../ttpdlg
  ../ttpcmn
  ${ONIGURUMA_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}/../ttpdlg     # for svnversion.h
  ${SFMT_INCLUDE_DIRS}
  )

if(MINGW)
  target_include_directories(
    teraterm
    PRIVATE
    .
    )
endif()

if(MSVC AND ENABLE_DEBUG_INFO)
  target_compile_options(
	teraterm
	PRIVATE
    $<$<CONFIG:Release>:/Zi>
    )
  if(${CMAKE_VERSION} VERSION_GREATER "3.13")
	target_link_options(
	  teraterm
	  PRIVATE
	  /DEBUG:FULL
	  /pdbaltpath:ttermpro.pdb
	  /map
	  )
  endif()
endif()

target_link_libraries(
  teraterm
  ttpcmn
  ttpfile
  ttpset
  ttptek
  ${ONIGURUMA_LIB}
  ${SFMT_LIB}
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

####
configure_file(
  svnversion.h.in
  ../ttpdlg/svnversion.h @ONLY)

install(
  TARGETS teraterm
  DESTINATION .
  )
if(MSVC AND ENABLE_DEBUG_INFO AND (${CMAKE_VERSION} VERSION_GREATER "3.13"))
  install(
    FILES
    $<TARGET_PDB_FILE:teraterm>
    $<$<CONFIG:Debug>:debug/ttermpro.map>
    $<$<CONFIG:Release>:release/ttermpro.map>
    DESTINATION .
    )
endif()
