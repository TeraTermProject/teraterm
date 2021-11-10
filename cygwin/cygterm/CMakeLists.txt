option(MSYS2TERM "msys2term" OFF)

if (MSYS2TERM)
  set(PACKAGE_NAME "msys2term")
else()
  set(PACKAGE_NAME "cygterm")
endif()
ENABLE_LANGUAGE(RC)
project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME}
  cygterm.cc
  cygterm.rc
  )

set_source_files_properties(
  cygterm.cc
  PROPERTIES COMPILE_FLAGS "-D_GNU_SOURCE -fno-exceptions"
)
