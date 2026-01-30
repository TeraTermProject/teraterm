# for libreSSL
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=win32 -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=x64   -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm64 -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm   -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=x86_64 -P buildlibressl.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildlibressl.cmake

include(script_support.cmake)

set(EXTRACT_DIR "${CMAKE_CURRENT_LIST_DIR}/build/libressl/src")
set(SRC_DIR "${EXTRACT_DIR}/libressl")
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/build/libressl/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/libressl_${TOOLSET}")

########################################

# Configure + Generate
function(cmake_generate GENERATOR SRC_DIR WORKING_DIR OPTIONS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G "${GENERATOR}" ${OPTIONS}
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

file(MAKE_DIRECTORY ${SRC_DIR})

execute_process(
  COMMAND ${CMAKE_COMMAND} -DTARGET=libressl -DEXT_DIR=${EXTRACT_DIR} -P download.cmake
)

if(${SRC_DIR}/COPYING IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/LibreSSL-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/COPYING
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/COPYING
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LibreSSL-LICENSE.txt)
endif()

function(file_copy SRC DEST_DIR DEST)
  set(SRC_FILE ${SRC})
  set(DEST_FILE ${DEST_DIR}/${DEST})
  file(SHA256 ${SRC_FILE} SRC_HASH)
  if (EXISTS ${DEST_FILE})
    file(SHA256 ${DEST_FILE} DEST_HASH)
  else()
    set(DEST_HASH "0")
  endif()
  if (NOT ${SRC_HASH} STREQUAL ${DEST_HASH})
    file(COPY ${SRC_FILE} DESTINATION ${DEST_DIR})
  endif()
endfunction()

file_copy(${SRC_DIR}/include/compat/stdlib.h ${CMAKE_CURRENT_LIST_DIR}/include/compat stdlib.h)

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  # multi-configuration
  unset(GENERATE_OPTIONS)
  if(DEFINED ARCHITECTURE)
    list(APPEND GENERATE_OPTIONS "-A" ${ARCHITECTURE})
  endif()
  list(APPEND GENERATE_OPTIONS "-DCMAKE_DEBUG_POSTFIX=d")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
  list(APPEND GENERATE_OPTIONS "-DLIBRESSL_TESTS=off")
  list(APPEND GENERATE_OPTIONS "-DMSVC=on")
  list(APPEND GENERATE_OPTIONS "-DUSE_STATIC_MSVC_RUNTIMES=on")

  cmake_generate("${CMAKE_GENERATOR}" "${SRC_DIR}" "${BUILD_DIR}" "${GENERATE_OPTIONS}")

  unset(BUILD_OPTIONS)
  list(APPEND BUILD_OPTIONS --config Debug)
  cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "")

  unset(BUILD_OPTIONS)
  list(APPEND BUILD_OPTIONS --config Release)
  cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "")
else()
  # single-configuration
  unset(GENERATE_OPTIONS)
  list(APPEND GENERATE_OPTIONS "-DARCHITECTURE=${ARCHITECTURE}")
  if(CMAKE_HOST_UNIX OR (DEFINED ENV{MSYSTEM}))
    list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/../mingw.toolchain.cmake")
  endif()
  if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio") OR ("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles"))
    list(APPEND GENERATE_OPTIONS "-DMSVC=on")
    list(APPEND GENERATE_OPTIONS "-DUSE_STATIC_MSVC_RUNTIMES=on")
  endif()
  list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_BUILD_TYPE=Release")
  list(APPEND GENERATE_OPTIONS "-DLIBRESSL_TESTS=off")

  cmake_generate("${CMAKE_GENERATOR}" "${SRC_DIR}" "${BUILD_DIR}" "${GENERATE_OPTIONS}")

  if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
    list(APPEND BUILD_TOOL_OPTIONS "-j")
  endif()

  unset(BUILD_OPTIONS)
  cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "${BUILD_TOOL_OPTIONS}")

endif()
