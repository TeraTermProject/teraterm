project(ttpfile)

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
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/ttlib.c
  ../common/ttlib.h
  bplus.c
  bplus.h
  ftlib.c
  ftlib.h
  kermit.c
  kermit.h
  quickvan.c
  quickvan.h
  ttfile.c
  xmodem.c
  xmodem.h
  ymodem.c
  ymodem.h
  zmodem.c
  zmodem.h
  file_res.h
  ttpfile-version.rc
  ttpfile.rc
  ttpfile.def
  )

include_directories(
  ../common
  )

add_library(
  ttpfile SHARED
  ${SRC}
  )

target_link_libraries(
  ttpfile
  ttpcmn
  )

add_dependencies(
  ttpfile
  ttpcmn
  )
