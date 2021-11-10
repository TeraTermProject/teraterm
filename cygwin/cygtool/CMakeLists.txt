cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "cygtool_dll")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  cygtool.c
  cygtool.def
  )

if(MSVC)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -MT
    )
endif()

if(MINGW)
  # libgcc_s_dw2-1.dll に依存しないため
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      -static-libgcc
      )
  else()
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
  endif()
endif()

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/libmingw/tlssup.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  version
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  OUTPUT_NAME "cygtool"
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  FOLDER installer
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

if(false)
  add_executable(
    cygtool_exe
    cygtool.c
    )

  target_compile_options(
    cygtool_exe
    PRIVATE
    -D EXE
    -I ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
    )

  target_link_libraries(
    cygtool_exe
    PRIVATE
    version
    )

  set_target_properties(
    cygtool_exe
    PROPERTIES
    OUTPUT_NAME "cygtool"
    )

  set_target_properties(
    cygtool_exe
    PROPERTIES
    FOLDER installer
    )
endif()

if(false)
  add_custom_target(
    cygtool ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cygtool.dll
    )

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cygtool.dll
    COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/cygtool/build_cygtool.cmake
    COMMAND ${CMAKE_COMMAND} -E echo ${CMAKE_GENERATOR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/cygtool.dll"
    )
endif()

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )
