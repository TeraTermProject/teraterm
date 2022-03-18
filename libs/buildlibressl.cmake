# for libreSSL
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P buildlibressl.cmake

include(script_support.cmake)

set(SRC_DIR_BASE "libressl-3.4.3")
set(SRC_ARC "libressl-3.4.3.tar.gz")
set(SRC_URL "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.4.3.tar.gz")
set(SRC_ARC_HASH_SHA256 ff88bffe354818b3ccf545e3cafe454c5031c7a77217074f533271d63c37f08d)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/libressl")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/libressl/src")
set(SRC_DIR "${EXTRACT_DIR}/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/libressl_${TOOLSET}")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/libressl/build_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("${CMAKE_COMMAND}" MATCHES "mingw64"))
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
  set(BUILD_DIR "${BUILD_DIR}_x64")
endif()

#message("BUILD_DIR=${BUILD_DIR}")
#message("INSTALL_DIR=${INSTALL_DIR}")

########################################

# Configure + Generate
function(cmake_generate GENERATOR SRC_DIR WORKING_DIR OPTIONS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${SRC_DIR} .. -G "${GENERATOR}" ${OPTIONS}
    WORKING_DIRECTORY "${BUILD_DIR}"
    ENCODING AUTO
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake build fail ${rv}")
  endif()
endfunction()

# build + install
function(cmake_build WORKING_DIR OPTIONS BUILD_TOOL_OPTIONS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . ${OPTIONS} --target install -- ${BUILD_TOOL_OPTIONS}
    WORKING_DIRECTORY "${BUILD_DIR}"
    ENCODING AUTO
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake build fail ${rv}")
  endif()
endfunction()

########################################

if(NOT EXISTS ${SRC_DIR}/README.md)

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

  file(COPY
    ${SRC_DIR}/COPYING
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help
    )
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/COPYING
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LibreSSL-LICENSE.txt)
endif()

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if(("${CMAKE_BUILD_TYPE}" STREQUAL "") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL ""))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    # multi-configuration

    unset(GENERATE_OPTIONS)
    list(APPEND GENERATE_OPTIONS -A ${ARCHITECTURE})
    list(APPEND GENERATE_OPTIONS "-DCMAKE_DEBUG_POSTFIX=d")
    list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
    list(APPEND GENERATE_OPTIONS "-DLIBRESSL_TESTS=off")
    if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
      list(APPEND GENERATE_OPTIONS "-DMSVC=on")
      list(APPEND GENERATE_OPTIONS "-DUSE_STATIC_MSVC_RUNTIMES=on")
    endif()

    cmake_generate("${CMAKE_GENERATOR}" "${SRC_DIR}" "${BUILD_DIR}" "${GENERATE_OPTIONS}")

    unset(BUILD_OPTIONS)
    list(APPEND BUILD_OPTIONS "--config Debug")
    cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "")

    unset(BUILD_OPTIONS)
    list(APPEND BUILD_OPTIONS "--config Release")
    cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "")

    return()
  else()
    # single-configuration
    unset(GENERATE_OPTIONS)
    if(CMAKE_HOST_UNIX)
      list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/../mingw.toolchain.cmake")
    endif(CMAKE_HOST_UNIX)
    list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
    list(APPEND GENERATE_OPTIONS "-DCMAKE_BUILD_TYPE=Release")
    if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio") OR ("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles"))
      list(APPEND GENERATE_OPTIONS "-DMSVC=on")
      list(APPEND GENERATE_OPTIONS "-DUSE_STATIC_MSVC_RUNTIMES=on")
    endif()

    cmake_generate("${CMAKE_GENERATOR}" "${SRC_DIR}" "${BUILD_DIR}" "${GENERATE_OPTIONS}")

    if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
      set(APPEND BUILD_TOOL_OPTIONS "-j")
    endif()

    unset(BUILD_OPTIONS)
    cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "${BUILD_TOOL_OPTIONS}")

  endif()
endif()
