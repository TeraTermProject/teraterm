# cmake -P buildputty.cmake

set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/putty")

execute_process(
  COMMAND ${CMAKE_COMMAND} -DTARGET=putty -P download.cmake
)

if(${SRC_DIR}/LICENSE IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/cJSON-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/LICENCE
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LICENCE
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/PuTTY-LICENSE.txt)
endif()
