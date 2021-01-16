set(PACKAGE_NAME "TTXCheckUpdate")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
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
  ../../teraterm/common/getcontent.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../teraterm/ttpdlg/svnversion.h
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif()

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
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_compile_definitions(
  TTXCheckUpdate
  PRIVATE
  -D CJSON_HIDE_SYMBOLS
  )

if(MSVC_VERSION STRLESS 1600)
  # VS2005(1400),2008(1500) (<2010(1600)) special define
  set_source_files_properties(
    ../../libs/cJSON/cJSON.c
    PROPERTIES
    COMPILE_FLAGS "-D NAN=0"
    )
endif()

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../../teraterm/ttpdlg     # for svnversion.h
  ../../libs/cJSON
  ../../teraterm/teraterm
  ../../teraterm/common
)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  #
  wininet.lib
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
