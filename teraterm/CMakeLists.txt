
add_subdirectory(ttpcmn)
set_target_properties(
  ttpcmn
  PROPERTIES FOLDER teraterm)

add_subdirectory(ttpset)
set_target_properties(
  ttpset
  PROPERTIES FOLDER teraterm)

add_subdirectory(ttpmacro)
set_target_properties(
  ttpmacro
  PROPERTIES FOLDER teraterm)

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
set_target_properties(
  keycodeW
  PROPERTIES FOLDER teraterm)

add_subdirectory(common)
set_target_properties(
  common_static
  PROPERTIES FOLDER teraterm)

if(SUPPORT_OLD_WINDOWS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_subdirectory(layer_for_unicode)
    set_target_properties(
      layer_for_unicode
      PROPERTIES FOLDER teraterm)
  else()
    message(FATAL_ERROR "64bit binary can not support 9x")
  endif()

  if(MINGW)
    add_subdirectory(libmingw)
    set_target_properties(
      mingw_msvcrt
      PROPERTIES FOLDER teraterm)
  endif(MINGW)
endif()
