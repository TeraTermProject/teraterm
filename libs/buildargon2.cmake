# cmake -P buildargon2.cmake

set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/argon2")

execute_process(
  COMMAND ${CMAKE_COMMAND} -DTARGET=argon2 -P download.cmake
)

if(${SRC_DIR}/LICENSE IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/argon2-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/LICENSE
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LICENSE
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/argon2-LICENSE.txt)
endif()
