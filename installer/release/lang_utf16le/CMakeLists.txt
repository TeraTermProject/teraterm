cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "lang_utf16le")
project(${PACKAGE_NAME})

if(NOT DEFINED BINARY_DIR)
  MESSAGE(FATAL_ERROR "check BINARY_DIR")
endif()

set(OUTPUT_DIR "${BINARY_DIR}/lang_utf16le/")

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

ConvertLNG(Default.lng utf16le)
ConvertLNG(en_US.lng utf16le)
ConvertLNG(fr_FR.lng utf16le)
ConvertLNG(de_DE.lng utf16le)
ConvertLNG(ja_JP.lng utf16le)
ConvertLNG(ko_KR.lng utf16le)
ConvertLNG(ru_RU.lng utf16le)
ConvertLNG("zh_CN.lng" utf16le)
ConvertLNG(es_ES.lng utf16le)
ConvertLNG("zh_TW.lng" utf16le)
ConvertLNG("ta_IN.lng" utf16le)
ConvertLNG("pt_BR.lng" utf16le)
ConvertLNG("it_IT.lng" utf16le)
ConvertLNG("tr_TR.lng" utf16le)

add_custom_target(
  ${PACKAGE_NAME} ALL
  DEPENDS ${CONVERTED_FILES}
  SOURCES ${SRC_FILES}
  SOURCES ${SCRIPT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

install(
  FILES ${CONVERTED_FILES}
  DESTINATION lang_utf16le/
  )
