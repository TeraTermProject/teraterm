cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "cygtool")

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

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

###### cygtool_tester

add_executable(
  cygtool_tester
  cygtool_tester.c
  )

target_compile_options(
  cygtool_tester
  PRIVATE
  -I ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

target_link_libraries(
  cygtool_tester
  PRIVATE
  ${PACKAGE_NAME}
  )
