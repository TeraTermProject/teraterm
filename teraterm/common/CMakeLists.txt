set(PACKAGE_NAME "common_static")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  STATIC
  asprintf.cpp
  asprintf.h
  codeconv.cpp
  codeconv.h
  compat_win.cpp
  compat_win.h
  comportinfo.cpp
  comportinfo.h
  dlglib.c
  dlglib.h
  dlglib_cpp.cpp
  dlglib_tmpl.cpp
  dllutil.cpp
  dllutil.h
  fileread.cpp
  fileread.h
  getcontent.cpp
  getcontent.h
  i18n.h
  i18n_static.c
  tipwin.cpp
  tipwin.h
  tmfc.cpp
  tmfc.h
  tmfc_frame.cpp
  tmfc_property.cpp
  ttlib.h
  ttlib_static.c
  ttlib_static_cpp.cpp
  win16api.c
  win16api.h
  win32helper.cpp
  win32helper.h
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PUBLIC
  #
  setupapi
  )
