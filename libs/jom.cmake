# cmake -P jom.cmake

set(SRC_ARC "jom_1_1_3.zip")
set(SRC_URL "http://download.qt.io/official_releases/jom/jom_1_1_3.zip")
set(SRC_ARC_HASH_SHA256 128fdd846fe24f8594eed37d1d8929a0ea78df563537c0c1b1861a635013fff8)
set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/jom")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/jom")

if(NOT EXISTS ${INSTALL_DIR}/nmake.exe)

  file(DOWNLOAD
    ${SRC_URL}
    ${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
    SHOW_PROGRESS
    )

  file(MAKE_DIRECTORY ${INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${INSTALL_DIR}
    )

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/jom.exe ${INSTALL_DIR}/nmake.exe
    )

endif()
