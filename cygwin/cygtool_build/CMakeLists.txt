# VS 32bit 以外のビルドシステムから cygtool をビルドする

cmake_minimum_required(VERSION 3.13) # for Generator-Expression

if(NOT ("${CMAKE_GENERATOR}" MATCHES "Visual Studio"))
  # 現在VS以外はうまくっていない
  return()
endif()

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(BINARY_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
  set(BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

set(SRC
  ${CMAKE_CURRENT_LIST_DIR}/../cygtool/cygtool.c
  ${CMAKE_CURRENT_LIST_DIR}/../cygtool/cygtool.h
  ${CMAKE_CURRENT_LIST_DIR}/../cygtool/cygtool.def
  )

add_custom_target(
  cygtool_build ALL
  DEPENDS ${BINARY_DIR}/cygtool.dll
  SOURCES ${SRC}
  )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build)

add_custom_command(
  OUTPUT ${BINARY_DIR}/cygtool.dll
  DEPENDS ${SRC}
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -DCMAKE_INSTALL_PREFIX=${BINARY_DIR} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/../cygtool/build_cygtool.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build
  )

install(
  FILES ${BINARY_DIR}/cygtool.dll
  DESTINATION .
  )
