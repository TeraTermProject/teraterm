project(TTXCheckUpdate)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_library(
  TTXCheckUpdate SHARED
  ttxcheckupdate.c
  parse.h
  parse.cpp
  getcontent.h
  getcontent.cpp
  resource.rc
  resource.h
  #
  cJSON/cJSON.h
  cJSON/cJSON.c
  #
  ../../teraterm/common/codeconv.cpp
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/dlglib.c
  ../../teraterm/common/dlglib_cpp.cpp
  ../../teraterm/common/dlglib_tmpl.cpp
  ../../teraterm/common/ttlib.c
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/"
  )

set_target_properties(
  TTXCheckUpdate
  PROPERTIES
  PREFIX ""
  )

target_include_directories(
  TTXCheckUpdate
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
)

target_link_libraries(
  TTXCheckUpdate
  PRIVATE
  ttpcmn
  Wininet.lib
  )
