
add_custom_target(
  teraterm_ini ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

set_target_properties(
  teraterm_ini
  PROPERTIES FOLDER installer
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI > "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build ${CMAKE_BINARY_DIR} --target teraterm_ini)")
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  DESTINATION .
  )

install(
  DIRECTORY release/
  DESTINATION .
  PATTERN *~ EXCLUDE
  PATTERN TERATERM.INI EXCLUDE
  PATTERN lang* EXCLUDE
  )

install(
  FILES release/EDITOR.CNF
  DESTINATION .
  RENAME KEYBOARD.CNF
  )

# Inno Setup
#  Create setup.exe
find_program(
  ISCC ISCC.exe
  HINTS "${CMAKE_SOURCE_DIR}/buildtools/innosetup6/bin"
  HINTS "C:/Program Files (x86)/Inno Setup 6/"
  HINTS "C:/Program Files (x86)/Inno Setup 5/"
  HINTS "C:/Program Files/Inno Setup 6/"
  HINTS "C:/Program Files/Inno Setup 5/"
  HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
  HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 5"
  )
message("ISCC=${ISCC}")

if(NOT TARGET ttxssh)
  add_custom_target(
    inno_setup
    COMMAND "${CMAKE_COMMAND}" -E echo "ttxssh was not build"
    )
elseif("${ISCC}" STREQUAL "ISCC-NOTFOUND")
  add_custom_target(
    inno_setup
    COMMAND "${CMAKE_COMMAND}" -E echo "inno setup not found"
    )
else()
  configure_file(
    teraterm_cmake.iss.in teraterm_cmake.iss
    NEWLINE_STYLE CRLF
    )

  if (RELEASE)
    set(ISCC_OPTION "-DSETUP_RELEASE=1")
  endif()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND ISCC_OPTION "-DM_X64=1")
  endif()
  add_custom_target(
    inno_setup
    DEPENDS cygtool
    COMMAND ${ISCC} ${ISCC_OPTION} teraterm_cmake.iss
    WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
    )
endif()
set_target_properties(
  inno_setup
  PROPERTIES FOLDER installer
  )

# zip
add_custom_target(
  zip
#  COMMAND "${CMAKE_COMMAND}" --build . --target install
  COMMAND "${CMAKE_COMMAND}" -E tar cvf ${SETUP_ZIP} --format=zip ${CMAKE_INSTALL_PREFIX}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

set_target_properties(
  zip
  PROPERTIES FOLDER installer
  )

add_subdirectory(release/lang)
set_target_properties(lang PROPERTIES FOLDER installer)
add_subdirectory(release/lang_utf16le)
set_target_properties(lang_utf16le PROPERTIES FOLDER installer)
