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

add_subdirectory(ttcomtester)
set_target_properties(
  ttcomtester
  PROPERTIES FOLDER tools
)
