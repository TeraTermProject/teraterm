
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

add_subdirectory(common)
set_target_properties(
  common_static
  PROPERTIES FOLDER teraterm)

