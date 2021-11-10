add_subdirectory(cyglaunch)
set_target_properties(
  cyglaunch
  PROPERTIES FOLDER cygwin)

add_subdirectory(cyglib)
set_target_properties(
  cyglib
  PROPERTIES FOLDER cygwin)

add_subdirectory(cygtool)
set_target_properties(
  cygtool_dll
  PROPERTIES FOLDER cygwin)

if(false)
  add_subdirectory(cygterm)
  set_target_properties(
    cygterm
    PROPERTIES FOLDER cygwin)
endif()

