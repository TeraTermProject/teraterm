project(ttpset)

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
  ../common/ttlib.c
  ../common/ttlib.h
  ttset.c
  ttpset-version.rc
  ttpset.def
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

if(${MINGW})
  link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../htmlhelp
	)
endif()

add_library(
  ttpset SHARED
  ${SRC}
  )

target_link_libraries(
  ttpset
  ttpcmn
  )

add_dependencies(
  ttpset
  ttpcmn
  )
