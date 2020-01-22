project(TTXCheckUpdate)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_library(
  TTXCheckUpdate SHARED
  ttxcheckupdate.c
  parse.h
  parse.cpp
  resource.rc
  resource.h
  #
  ../../libs/cJSON/cJSON.h
  ../../libs/cJSON/cJSON.c
  #
  ../../teraterm/common/codeconv.h
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/dlglib.c
  ../../teraterm/common/dlglib_cpp.cpp
  ../../teraterm/common/dlglib_tmpl.cpp
  ../../teraterm/common/ttlib.c
  ../../teraterm/common/getcontent.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/"
  )

source_group(
  "cJSON"
  REGULAR_EXPRESSION
  "..\/..\/libs\/cJSON\/"
  )
set_target_properties(
  TTXCheckUpdate
  PROPERTIES
  PREFIX ""
  )

target_compile_definitions(
  TTXCheckUpdate
  PRIVATE
  -D CJSON_HIDE_SYMBOLS)

target_include_directories(
  TTXCheckUpdate
  PRIVATE
  ../../libs/cJSON
  ../../teraterm/teraterm
  ../../teraterm/common
)

target_link_libraries(
  TTXCheckUpdate
  PRIVATE
  common_static
  ttpcmn
  Wininet.lib
  )

install(
  TARGETS TTXCheckUpdate
  RUNTIME
  DESTINATION .
  )
