# 32bit Windows 用 cygtool をビルドする

cmake_minimum_required(VERSION 3.13) # for Generator-Expression

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  # linux上ではcygwin関連はビルドしない
  return()
endif()

set(CYGTOOL_DIR "${CMAKE_CURRENT_LIST_DIR}/../cygtool")
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(BINARY_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
  set(BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

set(SRC
  ${CYGTOOL_DIR}/cygtool.c
  ${CYGTOOL_DIR}/cygtool.h
  ${CYGTOOL_DIR}/cygtool.def
  )

add_custom_target(
  cygtool ALL
  DEPENDS ${BINARY_DIR}/cygtool.dll
  DEPENDS ${SRC}
  SOURCES ${SRC}
  )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build)

add_custom_command(
  OUTPUT ${BINARY_DIR}/cygtool.dll
  DEPENDS ${SRC}
  COMMAND ${CMAKE_COMMAND}
    -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
    -DCMAKE_INSTALL_PREFIX=${BINARY_DIR}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -P ${CYGTOOL_DIR}/build_cygtool.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build
  )

install(
  FILES ${BINARY_DIR}/cygtool.dll
  DESTINATION .
  )
