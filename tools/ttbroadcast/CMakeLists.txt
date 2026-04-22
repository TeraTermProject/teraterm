project("ttbroadcast")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PROJECT_NAME}
  ../libs/getopt_mb_uni_src/getopt.c
  ../libs/getopt_mb_uni_src/getopt.h
  main.cpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common/asprintf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../teraterm/common/codeconv.h
  )

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
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
  ttpcmn
  common_static
  )

if(MINGW)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
    )
endif()
