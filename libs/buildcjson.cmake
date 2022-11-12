# cmake -P buildcjson.cmake

set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/cJSON")

execute_process(
  COMMAND ${CMAKE_COMMAND} -DTARGET=cjson -P download.cmake
)

if(${SRC_DIR}/LICENSE IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/cJSON-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/LICENSE
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LICENSE
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/cJSON-LICENSE.txt)
endif()
