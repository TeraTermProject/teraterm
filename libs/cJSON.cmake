# cmake -P cjson.cmake

set(SRC_DIR_BASE "cJSON-1.7.14")
set(SRC_ARC "v1.7.14.zip")
set(SRC_URL "https://github.com/DaveGamble/cJSON/archive/v1.7.14.zip")
set(SRC_ARC_HASH_SHA256 d797b4440c91a19fa9c721d1f8bab21078624aa9555fc64c5c82e24aa2a08221)
set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/cJSON")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/cJSON")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/cJSON/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/cJSON")

if(NOT EXISTS ${INSTALL_DIR}/README.md)

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

endif()
