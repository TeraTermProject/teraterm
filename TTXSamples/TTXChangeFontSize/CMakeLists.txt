cmake_minimum_required(VERSION 3.11)

project("TTXChangeFontSize")

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
)

add_library(
  ${PROJECT_NAME} SHARED
  TTXChangeFontSize.c
  ReadMe.txt
  ReadMe-ja.txt
  LICENSE
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
)

target_link_libraries(
  ${PROJECT_NAME}
  common_static
)

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME
  DESTINATION .
)
