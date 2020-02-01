# cmake -P perl.cmake

set(SRC_ARC "strawberry-perl-5.30.1.1-32bit.zip")
set(SRC_URL "http://strawberryperl.com/download/5.30.1.1/strawberry-perl-5.30.1.1-32bit.zip")
set(SRC_ARC_HASH_SHA256 cd9d7d5a3d0099752d6587770f4920b2b3818b16078e7b4dbf83f2aa2c037f70)
set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/perl")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/perl")

if(NOT EXISTS ${INSTALL_DIR}/perl/bin/perl.exe)

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

endif()
