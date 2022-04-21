# cygterm,msys2term をビルドする
#   - ${CMAKE_CURRENT_LIST_DIR}/../cygtool/build_cygterm.cmake を使ってビルド
#   - ${CMAKE_BINARY_DIR}/$<CONFIG> にビルドしたファイルを出力
#   - install で出力先にコピー

cmake_minimum_required(VERSION 3.13) # for Generator-Expression

set(CYGTERM_DIR "${CMAKE_CURRENT_LIST_DIR}/../cygterm")

file(GLOB SRC "${CYGTERM_DIR}/*.cpp" "${CYGTERM_DIR}/*.h")

add_custom_target(
  cygterm_build ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  DEPENDS ${SRC}
  SOURCES ${SRC}
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
  COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${BINARY_DIR} -P ${CYGTERM_DIR}/build_cygterm.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygterm_build
  )

install(
  FILES
    ${BINARY_DIR}/cygterm+-x86_64/cygterm.exe
    ${BINARY_DIR}/cygterm+-x86_64/cygterm.cfg
  DESTINATION cygterm+-x86_64
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/cygterm+-x86_64/cygterm.exe
    ${BINARY_DIR}/cygterm+-x86_64/cygterm.cfg
  DESTINATION .
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/cygterm+-i686/cygterm.exe
    ${BINARY_DIR}/cygterm+-i686/cygterm.cfg
  DESTINATION cygterm+-i686
  OPTIONAL
  )
install(
  FILES
    ${BINARY_DIR}/msys2term.exe
    ${BINARY_DIR}/msys2term.cfg
  DESTINATION .
  OPTIONAL
  )
