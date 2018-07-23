
project(ttptek)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  #
  tek_res.h
  tekesc.c
  tekesc.h
  ttptek.def
  ttptek.rc
  ttptek-version.rc
  tttek.c
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/oniguruma/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/SFMT
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )
link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/oniguruma/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/SFMT
  )

add_library(
  ttptek SHARED
  ${SRC}
  )

target_link_libraries(
  ttptek
  ttpcmn
  onig_s.lib
  SFMT.lib
  iphlpapi.lib
)
