set(PACKAGE_NAME "lscom")

project(${PACKAGE_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PACKAGE_NAME}
  main.cpp
  iswindowsntkernel.c
  #
  ../../teraterm/ttpcmn/ttcmn_cominfo.h
  ../../teraterm/ttpcmn/ttcmn_cominfo.c
  #
  ../libs/getopt_mb_uni_src/getopt.c
  ../libs/getopt_mb_uni_src/getopt.h
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/ttpcmn
  ${CMAKE_CURRENT_SOURCE_DIR}/../libs/getopt_mb_uni_src
  )

target_compile_definitions(
  ${PACKAGE_NAME}
  PRIVATE
  STATIC_GETOPT
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  #
  #setupapi
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

if(MINGW)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -municode
    )
endif()
