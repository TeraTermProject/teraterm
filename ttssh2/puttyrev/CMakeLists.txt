project(puttyrev)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  puttyrev.h
  puttyrev.cpp
  )

add_executable(
  puttyrev
  ${SRC}
  )
