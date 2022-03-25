# cygwin,msys2を使って cygterm,msys2term をビルドする

cmake_minimum_required(VERSION 3.13) # for Generator-Expression

add_custom_target(
  cygterm_build ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygterm_build)

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(BINARY_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
  set(BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  COMMAND ${CMAKE_COMMAND} -E echo ${BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${BINARY_DIR} -P ${CMAKE_CURRENT_LIST_DIR}/../cygterm/build_cygterm.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygterm_build
  )

install(
  FILES
    ${BINARY_DIR}/cygterm_x86_64/cygterm.exe
    ${BINARY_DIR}/cygterm_x86_64/cygterm.cfg
  DESTINATION cygterm_x86_64
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/cygterm_x86_64/cygterm.exe
    ${BINARY_DIR}/cygterm_x86_64/cygterm.cfg
  DESTINATION .
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/cygterm_i686/cygterm.exe
    ${BINARY_DIR}/cygterm_i686/cygterm.cfg
  DESTINATION cygterm_i686
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/msys2term.exe
    ${BINARY_DIR}/msys2term.cfg
  DESTINATION .
  OPTIONAL
  )
