
add_custom_target(
  cygterm_build ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygterm_build)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  COMMAND ${CMAKE_COMMAND} -E echo ${CMAKE_BINARY_DIR}/$<CONFIG>
  COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/$<CONFIG> -P ${CMAKE_CURRENT_LIST_DIR}/../cygterm/build_cygterm.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygterm_build
  )

install(
  FILES
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_x86_64/cygterm.exe
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_x86_64/cygterm.cfg
  DESTINATION cygterm_x86_64
  OPTIONAL
  )
install(
  FILES
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_x86_64/cygterm.exe
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_x86_64/cygterm.cfg
  DESTINATION .
  OPTIONAL
  )
install(
  FILES
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_i686/cygterm.exe
    ${CMAKE_BINARY_DIR}/$<CONFIG>/cygterm_i686/cygterm.cfg
  DESTINATION cygterm_i686
  OPTIONAL
  )
install(
  FILES
    ${CMAKE_BINARY_DIR}/$<CONFIG>/msys2term.exe
    ${CMAKE_BINARY_DIR}/$<CONFIG>/msys2term.cfg
  DESTINATION .
  OPTIONAL
  )
