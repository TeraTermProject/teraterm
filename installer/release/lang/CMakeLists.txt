cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "lang")
project(${PACKAGE_NAME})

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(BINARY_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
  set(BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()
set(OUTPUT_DIR "${BINARY_DIR}/lang/")

if(NOT DEFINED PERL)
  MESSAGE(FATAL_ERROR "check perl PERL=${PERL}")
endif()

set(CONVERTED_FILES "")
set(SRC_FILES "")
set(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../utf8_to.pl)

function(ConvertLNG FNAME OPTION)
  set(SRC ${CMAKE_CURRENT_LIST_DIR}/../lang_utf8/${FNAME})
  set(DEST ${OUTPUT_DIR}/${FNAME})
  list(APPEND CONVERTED_FILES ${DEST})
  list(APPEND SRC_FILES ${SRC})
  add_custom_command(
    OUTPUT ${DEST}
    DEPENDS ${SRC}
    DEPENDS ${SCRIPT}
    COMMAND ${PERL} ${SCRIPT} ${OPTION} ${SRC} ${DEST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
  set(CONVERTED_FILES ${CONVERTED_FILES} PARENT_SCOPE)
  set(SRC_FILES ${SRC_FILES} PARENT_SCOPE)
endfunction()

ConvertLNG(Default.lng cp1252)
ConvertLNG(English.lng cp1252)
ConvertLNG(French.lng cp1252)
ConvertLNG(German.lng cp1252)
ConvertLNG(Japanese.lng cp932)
ConvertLNG(Korean.lng cp949)
ConvertLNG(Russian.lng windows-1251)
ConvertLNG("Simplified Chinese.lng" cp936)
ConvertLNG(Spanish.lng cp1252)
ConvertLNG("Traditional Chinese.lng" cp950)

add_custom_target(
  ${PACKAGE_NAME} ALL
  DEPENDS ${CONVERTED_FILES}
  SOURCES ${SRC_FILES}
  SOURCES ${SCRIPT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

install(
  FILES ${CONVERTED_FILES}
  DESTINATION lang/
  )
