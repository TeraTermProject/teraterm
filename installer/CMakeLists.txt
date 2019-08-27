
install(
  DIRECTORY release/
  DESTINATION .
  PATTERN *~ EXCLUDE
  PATTERN TERATERM.INI EXCLUDE
  )

install(
  FILES release/EDITOR.CNF
  DESTINATION .
  RENAME KEYBOARD.CNF
  )

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  find_program(
    PERL perl
    )
else()
  # CYGWIN / Active Perl
  find_program(
    PERL perl.exe
    HINTS c:/Perl64/bin
    HINTS c:/Perl/bin
    HINTS c:/cygwin/usr/bin
    HINTS c:/cygwin64/usr/bin
    )
endif()

add_custom_target(
  teraterm_ini ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI > "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  DESTINATION .
  )

