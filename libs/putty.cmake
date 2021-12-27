# cmake -P putty.cmake

set(SRC_DIR_BASE "putty-0.76")
set(SRC_ARC "putty-0.76.tar.gz")
set(SRC_URL "https://the.earth.li/~sgtatham/putty/0.76/putty-0.76.tar.gz")
set(SRC_ARC_HASH_SHA256 547cd97a8daa87ef71037fab0773bceb54a8abccb2f825a49ef8eba5e045713f)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/putty")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/putty/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/putty/src/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/putty")

if(NOT EXISTS ${INSTALL_DIR}/README)

  file(DOWNLOAD
    ${SRC_URL}
    ${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
    SHOW_PROGRESS
    )

  file(MAKE_DIRECTORY ${EXTRACT_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )

  file(REMOVE_RECURSE ${INSTALL_DIR})
  file(RENAME ${SRC_DIR} ${INSTALL_DIR})
  file(REMOVE_RECURSE ${EXTRACT_DIR})

  file(COPY
    ${INSTALL_DIR}/LICENCE
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LICENCE
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/PuTTY-LICENSE.txt)
endif()
