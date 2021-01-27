set(PACKAGE_NAME "mingw_msvcrt")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  STATIC
  msvcrt_wrapper.c
  )
