cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "lang")
project(${PACKAGE_NAME})

if(NOT DEFINED BINARY_DIR)
  MESSAGE(FATAL_ERROR "check BINARY_DIR")
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
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${PERL} ${SCRIPT} ${OPTION} ${SRC} ${DEST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
  set(CONVERTED_FILES ${CONVERTED_FILES} PARENT_SCOPE)
  set(SRC_FILES ${SRC_FILES} PARENT_SCOPE)
endfunction()

ConvertLNG(Default.lng cp1252)
ConvertLNG(en_US.lng cp1252)
ConvertLNG(fr_FR.lng cp1252)
ConvertLNG(de_DE.lng cp1252)
ConvertLNG(ja_JP.lng cp932)
ConvertLNG(ko_KR.lng cp949)
ConvertLNG(ru_RU.lng windows-1251)
ConvertLNG("zh_CN.lng" cp936)
ConvertLNG(es_ES.lng cp1252)
ConvertLNG("zh_TW.lng" cp950)
ConvertLNG("ta_IN.lng" cp65001)
ConvertLNG("pt_BR.lng" cp1252)
ConvertLNG("it_IT.lng" cp1252)
ConvertLNG("tr_TR.lng" cp1254)

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
