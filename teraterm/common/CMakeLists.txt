project(teraterm)

add_library(
  common_static
  STATIC
  codeconv.cpp
  codeconv.h
  compat_win.cpp
  compat_win.h
  dllutil.cpp
  dllutil.h
  i18n.c
  i18n.h
  layer_for_unicode.cpp
  layer_for_unicode.h
  layer_for_unicode_comctl32.cpp
  tmfc.cpp
  tmfc.h
  tmfc_frame.cpp
  tmfc_property.cpp
  )

target_include_directories(
  common_static
  PRIVATE
  .
  )
