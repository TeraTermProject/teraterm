
set(PACKAGE_NAME "chm_ja")
project(${PACKAGE_NAME})

if(NOT DEFINED BINARY_DIR)
  MESSAGE(FATAL_ERROR "check BINARY_DIR")
endif()

set(CONVERTED_FILES "")

function(ConvertHTML output depend options)
  list(APPEND CONVERTED_FILES ${CMAKE_CURRENT_LIST_DIR}/${output})
  set(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../2sjis.pl)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${output}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${depend}
    DEPENDS ${SCRIPT}
    COMMAND ${PERL} ${SCRIPT}
    -i ${CMAKE_CURRENT_LIST_DIR}/${depend}
    -o ${CMAKE_CURRENT_LIST_DIR}/${output}
    ${options} --no_utime
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
  set(CONVERTED_FILES ${CONVERTED_FILES} PARENT_SCOPE)
endfunction()

set(REF "html/reference")
ConvertHTML(${REF}/Oniguruma-LICENSE.txt ../../libs/doc_help/Oniguruma-LICENSE.txt "-l;unix")
ConvertHTML(${REF}/RE.txt                ../../libs/doc_help/ja/RE                 "-l;unix;-c;utf8")
ConvertHTML(${REF}/LibreSSL-LICENSE.txt  ../../libs/doc_help/LibreSSL-LICENSE.txt  "-l;unix")
ConvertHTML(${REF}/PuTTY-LICENSE.txt     ../../libs/doc_help/PuTTY-LICENSE.txt     "-l;crlf")
ConvertHTML(${REF}/SFMT-LICENSE.txt      ../../libs/doc_help/SFMT-LICENSE.txt      "-l;unix")
ConvertHTML(${REF}/argon2-LICENSE.txt    ../../libs/doc_help/argon2-LICENSE.txt    "-l;unix")
ConvertHTML(${REF}/zlib-LICENSE.txt      ../../libs/doc_help/zlib-LICENSE.txt      "-l;unix;--zlib_special")
ConvertHTML(${REF}/cJSON-LICENSE.txt     ../../libs/doc_help/cJSON-LICENSE.txt     "-l;crlf")
ConvertHTML(${REF}/CygTerm+-LICENSE.txt  ../../cygwin/cygterm/COPYING              "-l;unix")
ConvertHTML(${REF}/build_with_cmake.html          ${REF}/build_with_cmake.md "")
ConvertHTML(${REF}/build_library_with_cmake.html  ${REF}/build_library_with_cmake.md "")
ConvertHTML(${REF}/menu_id.html                   ${REF}/menu_id.md "")
ConvertHTML(html/setup/folder.html                html/setup/folder.md "")

file(
  GLOB_RECURSE
  SRC
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  *.html *.md *.png
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SRC})

add_custom_target(
  ${PACKAGE_NAME} ALL
  DEPENDS ${BINARY_DIR}/teratermj.chm
  SOURCES ${SRC}
  SOURCES ../htmlhelp_index_make.pl
  SOURCES ../2sjis.pl
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

if(NOT("${HHC}" STREQUAL "HHC-NOTFOUND"))
  set(HHC_COMMAND ${HHC})
else()
  set(HHC_COMMAND ${CHMCMD})
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
  set(RM rm)
else()
  # 3.11は rm がない。remove を使用する
  set(RM remove)
endif()

add_custom_command(
  OUTPUT ${BINARY_DIR}/teratermj.chm
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Index.hhk
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/teraterm.hhc
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/teraterm.hhp
  DEPENDS ${CONVERTED_FILES}
  DEPENDS ${SRC}
  COMMAND ${CMAKE_COMMAND} -E ${RM} -f ${BINARY_DIR}/teratermj.chm
  COMMAND ${CMAKE_COMMAND} -DHHC=${HHC_COMMAND} -P ../chm.cmake
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_LIST_DIR}/teratermj.chm ${BINARY_DIR}/teratermj.chm
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_LIST_DIR}/Index.hhk
  DEPENDS ${SRC}
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/../htmlhelp_index_make.pl
  COMMAND ${PERL} ${CMAKE_CURRENT_LIST_DIR}/../htmlhelp_index_make.pl . html -o Index.hhk
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

install(
  FILES ${BINARY_DIR}/teratermj.chm
  DESTINATION .
)
