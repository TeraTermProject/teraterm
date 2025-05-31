cmake_minimum_required(VERSION 3.8)

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
  TTXChangeFontSize/TTXChangeFontSize.c
  TTXChangeFontSize/ReadMe.txt
  TTXChangeFontSize/ReadMe-ja.txt
  TTXChangeFontSize/LICENSE
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  TT_VERSION_MAJOR=5)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_link_libraries(
  ${PROJECT_NAME}
  common_static
  #
#  gdi32
  )

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME
  DESTINATION .
)
