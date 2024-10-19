cmake_minimum_required(VERSION 3.8)

set(PACKAGE_NAME "TTXChangeFontSize")
project(${PACKAGE_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
)

add_library(
  ${PACKAGE_NAME} SHARED
  TTXChangeFontSize/TTXChangeFontSize.c
  TTXChangeFontSize/ReadMe.txt
  TTXChangeFontSize/ReadMe-ja.txt
  TTXChangeFontSize/LICENSE
)

target_compile_definitions(${PACKAGE_NAME}
  PRIVATE
  TT_VERSION_MAJOR=5)

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  ${PACKAGE_NAME}
  common_static
  #
#  gdi32
  )
