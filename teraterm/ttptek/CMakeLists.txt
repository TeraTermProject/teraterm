project(ttptek)

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
  optimized onig_s.lib
  debug onig_sd.lib
  optimized SFMT.lib
  debug SFMTd.lib
  iphlpapi.lib
  )

add_dependencies(
  ttptek
  ttpcmn
  )
