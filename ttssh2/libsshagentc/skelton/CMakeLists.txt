project("libsshagentc")

add_library(
  ${PROJECT_NAME}
  ../libputty.h
  skelton.c
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  ..
)
