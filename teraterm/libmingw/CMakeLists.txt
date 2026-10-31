set(PACKAGE_NAME "mingw_msvcrt")

project(mingw)

add_library(
  mingw_msvcrt
  STATIC
  msvcrt_wrapper.c
  new_delete.cpp
  functexcept.cpp
)

# for dll tls
add_library(
  mingw_dll_tls
  STATIC
  tlssup.c
)
