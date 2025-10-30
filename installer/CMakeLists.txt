
add_custom_target(
  teraterm_ini ALL
  DEPENDS ${BINARY_DIR}/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
)

set_target_properties(teraterm_ini PROPERTIES FOLDER installer)

add_custom_command(
  OUTPUT ${BINARY_DIR}/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI > "${BINARY_DIR}/TERATERM.INI"
)

install(
  FILES ${BINARY_DIR}/TERATERM.INI
  DESTINATION .
)

add_custom_target(
  special_font ALL
  DEPENDS ${BINARY_DIR}/TSPECIAL1.TTF
)

add_custom_command(
  OUTPUT ${BINARY_DIR}/TSPECIAL1.TTF
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/release/TSPECIAL1.TTF ${BINARY_DIR}
)

set_target_properties(special_font PROPERTIES FOLDER installer)

install(
  DIRECTORY release/
  DESTINATION .
  PATTERN *~ EXCLUDE
  PATTERN TERATERM.INI EXCLUDE
  PATTERN lang* EXCLUDE
)

install(
  FILES release/IBMKEYB.CNF
  DESTINATION .
  RENAME KEYBOARD.CNF
)

install(
  FILES ../ttpmenu/readme.txt
  DESTINATION .
  RENAME ttmenu_readme-j.txt
)

add_subdirectory(release/lang)
set_target_properties(lang PROPERTIES FOLDER installer)
add_subdirectory(release/lang_utf16le)
set_target_properties(lang_utf16le PROPERTIES FOLDER installer)
