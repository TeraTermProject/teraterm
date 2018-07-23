
project(ttpset)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../common/ttlib.c
  ../common/ttlib.h
  ttset.c
  ttpset-version.rc
  ttpset.def
  )


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

add_library(
  ttpset SHARED
  ${SRC}
  )

target_link_libraries(
  ttpset
  ttpcmn
  )
