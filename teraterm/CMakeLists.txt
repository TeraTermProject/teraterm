
add_subdirectory(ttpcmn)
set_target_properties(
  ttpcmn
  PROPERTIES FOLDER teraterm)

add_subdirectory(ttpfile)
set_target_properties(
  ttpfile
  PROPERTIES FOLDER teraterm)

add_subdirectory(ttpset)
set_target_properties(
  ttpset
  PROPERTIES FOLDER teraterm)

if((NOT(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005" AND ${CMAKE_MAKE_PROGRAM} MATCHES "VCExpress.exe")) AND (NOT(MINGW)))
  # MFCを使用しているため VS2005 Express ではビルドできない
  add_subdirectory(ttpmacro)
  set_target_properties(
    ttpmacro
    PROPERTIES FOLDER teraterm)
endif()

add_subdirectory(ttptek)
set_target_properties(
  ttptek
  PROPERTIES FOLDER teraterm)

add_subdirectory(teraterm)
set_target_properties(
  teraterm
  PROPERTIES FOLDER teraterm)

add_subdirectory(keycode)
set_target_properties(
  keycode
  PROPERTIES FOLDER teraterm)
