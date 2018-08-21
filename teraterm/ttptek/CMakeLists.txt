project(ttptek)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_oniguruma.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_SFMT.cmake)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${ONIGURUMA_INCLUDE}
  ${SFMT_INCLUDE_DIR}
  )

link_directories(
  ${SFMT_LIBRARY_DIRS}
  )

if(MINGW)
  link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../htmlhelp
	)
endif()

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
  debug sfmtd.lib
  optimized sfmt.lib
  iphlpapi.lib
  htmlhelp.lib
  )

add_dependencies(
  ttptek
  ttpcmn
  )
