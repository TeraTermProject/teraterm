project(ttpcmn)

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(COMMON_SRC
  ../common/teraterm.h
  ../common/tttypes.h
  ../common/i18n.c
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/servicenames.c
  ../common/servicenames.h
  ../common/tt-version.h
  ../common/ttftypes.h
  ../common/ttcommon.h
  ../common/tt_res.h
  ../common/codeconv.h
  ../common/codeconv.cpp
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  language.c
  language.h
  ttcmn.c
  ttpcmn-version.rc
  ttpcmn.def
  ${COMMON_SRC}
  )

include_directories(
  ../common
  ../ttpfile
  .
  )

if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} /DELAYLOAD:setupapi.dll /DELAYLOAD:user32.dll")
endif()

add_library(
  ttpcmn
  SHARED
  ${SRC}
  )

target_compile_definitions(
  ttpcmn
  PRIVATE -DTTPCMN_DLL
  )
if(MINGW)
  set_target_properties(
    ttpcmn
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ttpcmn
  ole32
  setupapi
  delayimp
  )

install(
  TARGETS ttpcmn
  RUNTIME
  DESTINATION .
  )
