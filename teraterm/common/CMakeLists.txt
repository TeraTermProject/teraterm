set(PACKAGE_NAME "common_static")

project(${PACKAGE_NAME})

# Platform-independent sources
set(COMMON_SOURCES
  asprintf.cpp
  asprintf.h
  codeconv.h
  fileread.cpp
  fileread.h
  history_store.cpp
  history_store.h
  inifile_com.cpp
  inifile_com.h
  makeoutputstring.cpp
  makeoutputstring.h
  ttdebug.cpp
  ttdebug.h
  ttlib_types.cpp
  ttlib_types.h
  tttext.cpp
  tttext.h
  tttypes.h
  tttypes_charset.h
  tttypes_key.h
  tttypes_termid.cpp
  tttypes_termid.h
)

if(APPLE)
  # macOS build: only platform-independent + shim sources
  add_library(
    ${PACKAGE_NAME}
    STATIC
    ${COMMON_SOURCES}
    ../teraterm/unicode.cpp
    ../teraterm/unicode.h
    codeconv.cpp
    codeconv_mb.cpp
    codeconv_mb.h
    ttlib_charset.cpp
    ttlib_charset.h
  )

  target_include_directories(
    ${PACKAGE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}       # for svnversion.h
    .
    ../teraterm
    ../platform
    ../platform/macos
  )

  target_link_libraries(
    ${PACKAGE_NAME}
    PUBLIC
    platform
  )

else()
  # Windows build: full source set
  add_library(
    ${PACKAGE_NAME}
    STATIC
    ${COMMON_SOURCES}
    ../teraterm/unicode.cpp
    ../teraterm/unicode.h
    codeconv.cpp
    codeconv_mb.cpp
    codeconv_mb.h
    compat_win.cpp
    compat_win.h
    compat_dwrite.h
    comportinfo.cpp
    comportinfo.h
    ddelib.cpp
    ddelib.h
    dlglib.c
    dlglib.h
    dlglib_cpp.cpp
    dlglib_tmpl.cpp
    dllutil.cpp
    dllutil.h
    directx.h
    directx.cpp
    getcontent.cpp
    getcontent.h
    i18n.c
    i18n.h
    i18n_static.c
    resize_helper.cpp
    resize_helper.h
    tipwin.cpp
    tipwin.h
    tipwin2.cpp
    tipwin2.h
    tmfc.cpp
    tmfc.h
    tmfc_frame.cpp
    tmfc_propdlg.cpp
    tmfc_propdlg.h
    tmfc_property.cpp
    ttcommdlg.cpp
    ttcommdlg.h
    ttdup.cpp
    ttdup.h
    ttgdiplus.cpp
    ttgdiplus.h
    ttknownfolders.c
    ttknownfolders.h
    ttlib.c
    ttlib.h
    ttlib_charset.cpp
    ttlib_charset.h
    ttlib_static.c
    ttlib_static_cpp.cpp
    ttlib_static_dir.cpp
    ttlib_static_dir.h
    win32helper.cpp
    win32helper.h
  )

  target_include_directories(
    ${PACKAGE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}       # for svnversion.h
    .
    ../teraterm
  )

  if(SUPPORT_OLD_WINDOWS)
    target_link_libraries(
      ${PACKAGE_NAME}
      PUBLIC
      layer_for_unicode
    )
  endif()

  target_link_libraries(
    ${PACKAGE_NAME}
    PUBLIC
    #
    setupapi
    imagehlp
  )
endif()
