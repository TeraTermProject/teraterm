set(PACKAGE_NAME "libsshagentc")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  ../libputty.h
  skelton.c
)

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ..
)
