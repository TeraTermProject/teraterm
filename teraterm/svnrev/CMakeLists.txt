project(svnrev)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  svnrev.cpp
  svnrev.h
  )

include_directories(
  ../common
  ../ttpfile
  .
  )

add_executable(
  svnrev
  ${SRC}
  )

target_link_libraries(
  svnrev
  )
