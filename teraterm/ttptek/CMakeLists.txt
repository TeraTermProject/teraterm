project(ttptek)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/tttypes.h
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/codeconv.h
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
  PRIVATE
  common_static
  ttpcmn
  #
  iphlpapi
  gdi32
  )

install(
  TARGETS ttptek
  RUNTIME
  DESTINATION .
  )
