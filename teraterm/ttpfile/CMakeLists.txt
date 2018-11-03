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

set(COMMON_SRC
  ../common/dlglib.c
  ../common/dlglib.h
  ../common/dlglib_cpp.cpp
  ../common/dlglib_tmpl.cpp
  ../common/ttlib.c
  ../common/ttlib.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
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
  ${COMMON_SRC}
  )

include_directories(
  ../common
  )

if(MINGW)
  link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../htmlhelp
	)
endif()

add_library(
  ttpfile SHARED
  ${SRC}
  )

if(MINGW)
  set_target_properties(
	ttpfile
	PROPERTIES PREFIX ""
	SUFFIX ".dll"
	)
endif()

target_link_libraries(
  ttpfile
  ttpcmn
  #
  gdi32
  comdlg32
  )

add_dependencies(
  ttpfile
  ttpcmn
  )
