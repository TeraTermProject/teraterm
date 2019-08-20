project(ttptek)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/tttypes.h
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/codeconv.h
  ../common/codeconv.cpp
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  tekesc.c
  tekesc.h
  ttptek.def
  ttptek-version.rc
  tttek.h
  tttek.c
  ${COMMON_SRC}
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${ONIGURUMA_INCLUDE}
  ${SFMT_INCLUDE_DIR}
  )

link_directories(
  ${SFMT_LIBRARY_DIRS}
  )

add_library(
  ttptek SHARED
  ${SRC}
  )

if(MINGW)
  set_target_properties(
    ttptek
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ttptek
  ttpcmn
  debug sfmtd
  optimized sfmt
  #
  iphlpapi
  gdi32
  )

add_dependencies(
  ttptek
  ttpcmn
  )

install(
  TARGETS ttptek
  RUNTIME
  DESTINATION .
  )
