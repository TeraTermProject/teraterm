# cmake -P argon2.cmake

set(SRC_DIR_BASE "phc-winner-argon2-20190702")
set(SRC_ARC "20190702.tar.gz")
set(SRC_URL "https://github.com/P-H-C/phc-winner-argon2/archive/refs/tags/20190702.tar.gz")
set(SRC_ARC_HASH_SHA256 daf972a89577f8772602bf2eb38b6a3dd3d922bf5724d45e7f9589b5e830442c)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/argon2")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/argon2/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/argon2/src/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/argon2")

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
