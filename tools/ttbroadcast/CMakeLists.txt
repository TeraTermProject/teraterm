project(ttbroadcast)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ttbroadcast
  getopt_mb_uni_src/getopt.c
  getopt_mb_uni_src/getopt.h
  main.cpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common/asprintf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common/codeconv.h
  )

target_include_directories(
  ttbroadcast
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common
  ${CMAKE_CURRENT_SOURCE_DIR}/getopt_mb_uni_src
  )

target_compile_definitions(
  ttbroadcast
  PRIVATE
  STATIC_GETOPT
  )

target_link_libraries(
  ttbroadcast
  PRIVATE
  common_static
  )

if(MINGW)
  target_link_options(
    ttbroadcast
    PRIVATE
    -municode
    )
endif()
