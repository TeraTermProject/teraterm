add_subdirectory(ttbroadcast)
set_target_properties(
  ttbroadcast
  PROPERTIES FOLDER tools
)

add_subdirectory(lscom)
set_target_properties(
  lscom
  PROPERTIES FOLDER tools
)

add_subdirectory(ttreg)
set_target_properties(
  ttreg
  PROPERTIES FOLDER tools
)

add_subdirectory(ttlaunch)
set_target_properties(
  ttlaunch ttlaunch_lng
  PROPERTIES FOLDER tools
)
