project(matcher)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  matcher.c
  test.c
  )

add_executable(
  matcher
  ${SRC}
  )
