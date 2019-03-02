project(ttpset)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/tttypes.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/servicenames.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  ttset.c
  ttpset-version.rc
  ttpset.def
  ${COMMON_SRC}
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

add_library(
  ttpset SHARED
  ${SRC}
  )

if(MINGW)
  set_target_properties(
    ttpset
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ttpset
  ttpcmn
  #
  gdi32
  )

add_dependencies(
  ttpset
  ttpcmn
  )
