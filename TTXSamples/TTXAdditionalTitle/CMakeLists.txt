project(TTXAdditionalTitle)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXAdditionalTitle.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

add_library(
  TTXAdditionalTitle SHARED
  ${SRC}
  )

set_target_properties(
  TTXAdditionalTitle
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  TTXAdditionalTitle
  ttpcmn
  )

