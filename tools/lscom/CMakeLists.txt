project("lscom")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PROJECT_NAME}
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
  ${PROJECT_NAME}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/ttpcmn
  ${CMAKE_CURRENT_SOURCE_DIR}/../libs/getopt_mb_uni_src
  )

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
  STATIC_GETOPT
  )

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
)

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    layer_for_unicode
  )
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

if(MINGW)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
    )
endif()
