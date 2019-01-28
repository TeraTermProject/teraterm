project(ttpcmn)

if (MSVC)
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG            ${CMAKE_C_FLAGS_DEBUG})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_MINSIZEREL       ${CMAKE_C_FLAGS_MINSIZEREL})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE          ${CMAKE_C_FLAGS_RELEASE})
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO   ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_MINSIZEREL     ${CMAKE_CXX_FLAGS_MINSIZEREL})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_CXX_FLAGS_RELEASE})
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif ()

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
  cmn_res.h
  language.c
  language.h
  ttcmn.c
  ttcmn_notify.c
  ttpcmn-version.rc
  ${COMMON_SRC}
  )

include_directories(
  ../common
  ../ttpfile
  .
  )

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
  )
