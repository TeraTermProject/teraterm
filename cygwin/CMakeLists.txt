add_subdirectory(cyglaunch)
set_target_properties(
  cyglaunch
  PROPERTIES FOLDER cygwin)

add_subdirectory(cygtool)
set_target_properties(
  cygtool_dll
  PROPERTIES FOLDER cygwin)
