set(PACKAGE_NAME "cyglib")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  cyglib.c
  cyglib.h
  )

target_include_directories(
  ${PACKAGE_NAME}
  PUBLIC
  .
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PUBLIC
  common_static
  )
