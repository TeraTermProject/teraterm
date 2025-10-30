# for SFMT
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=win32 -P buildsfmt.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=x64   -P buildsfmt.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm64 -P buildsfmt.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm   -P buildsfmt.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=x86_64 -P buildsfmt.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildsfmt.cmake

include(script_support.cmake)

set(EXTRACT_DIR "${CMAKE_CURRENT_LIST_DIR}/build/SFMT/src")
set(SRC_DIR "${EXTRACT_DIR}/SFMT")
set(INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/SFMT_${TOOLSET}")
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/build/SFMT/build_${TOOLSET}")

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
  COMMAND ${CMAKE_COMMAND} -DTARGET=sfmt -DEXT_DIR=${EXTRACT_DIR} -P download.cmake
)

if(${SRC_DIR}/COPYING IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/SFMT-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/LICENSE.txt
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/LICENSE.txt
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/SFMT-LICENSE.txt)
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/CMakeLists.txt)
  file(WRITE "${SRC_DIR}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.11.4)\n"
    "project(SFMT C)\n"
    "\n"
    "if(MSVC)\n"
    "  set(CMAKE_DEBUG_POSTFIX \"d\")\n"
    "  string(REPLACE \"/MD\" \"/MT\" CMAKE_C_FLAGS_DEBUG \${CMAKE_C_FLAGS_DEBUG})\n"
    "  string(REPLACE \"/MD\" \"/MT\" CMAKE_C_FLAGS_RELEASE \${CMAKE_C_FLAGS_RELEASE})\n"
    "endif()\n"
    "\n"
    "add_library(\n"
    "  sfmt STATIC\n"
    "  SFMT.c\n"
    "  )\n"
    "\n"
    "if(MSVC)\n"
    "  target_compile_options(sfmt\n"
    "    PRIVATE\n"
    "    $<$<CONFIG:>:/MT>\n"
    "    $<$<CONFIG:Debug>:/MTd>\n"
    "    $<$<CONFIG:Release>:/MT>)\n"
    "endif()\n"
    "\n"
    "install(\n"
    "  TARGETS sfmt\n"
    "  ARCHIVE DESTINATION \${CMAKE_INSTALL_PREFIX}/lib\n"
    "  )\n"
    "install(\n"
    "  FILES\n"
    "    SFMT.h SFMT-params.h SFMT-params19937.h\n"
    "    SFMT_version_for_teraterm.h\n"
    "  DESTINATION \${CMAKE_INSTALL_PREFIX}/include\n"
    "  )\n"
    )
endif()

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  # multi-configuration
  unset(GENERATE_OPTIONS)
  if(DEFINED ARCHITECTURE)
    list(APPEND GENERATE_OPTIONS "-A" ${ARCHITECTURE})
  endif()
  list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/VSToolchain.cmake")
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
  if(CMAKE_HOST_UNIX)
    list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/../mingw.toolchain.cmake")
  endif(CMAKE_HOST_UNIX)
  if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio") OR ("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles"))
    list(APPEND GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/VSToolchain.cmake")
  endif()
  list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
  list(APPEND GENERATE_OPTIONS "-DCMAKE_BUILD_TYPE=Release")

  cmake_generate("${CMAKE_GENERATOR}" "${SRC_DIR}" "${BUILD_DIR}" "${GENERATE_OPTIONS}")

  if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
    list(APPEND BUILD_TOOL_OPTIONS "-j")
  endif()

  unset(BUILD_OPTIONS)
  cmake_build("${BUILD_DIR}" "${BUILD_OPTIONS}" "${BUILD_TOOL_OPTIONS}")

endif()
