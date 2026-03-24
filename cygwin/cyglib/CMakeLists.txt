project("cyglib")

add_library(
  ${PROJECT_NAME}
  cyglib.c
  cyglib.h
  )

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  .
  )

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  common_static
  )
