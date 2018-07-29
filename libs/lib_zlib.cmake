
string(REPLACE " " "_" GENERATOR_ ${CMAKE_GENERATOR})

list(
  APPEND
  CMAKE_PREFIX_PATH
  "${CMAKE_CURRENT_LIST_DIR}/zlib_${GENERATOR_}"
  )
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZLIB REQUIRED zlib)

