add_subdirectory(cyglaunch)
set_target_properties(
  cyglaunch
  PROPERTIES FOLDER cygwin)

add_subdirectory(cyglib)
set_target_properties(
  cyglib
  PROPERTIES FOLDER cygwin)

add_subdirectory(cygtool_build)
set_target_properties(
  cygtool_build
  PROPERTIES FOLDER cygwin)

add_subdirectory(cygterm_build)
set_target_properties(
  cygterm_build
  PROPERTIES FOLDER cygwin)
