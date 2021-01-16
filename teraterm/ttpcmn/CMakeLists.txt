set(PACKAGE_NAME "ttpcmn")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  SHARED
  language.c
  language.h
  ttcmn.c
  ttcmn_notify.cpp
  comportinfo.cpp
  comportinfo.h
  comportinfo.cpp
  devpkey_teraterm.h
  ttpcmn-version.rc
  ttpcmn.def
  ${COMMON_SRC}
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../common/teraterm.h
  ../common/tttypes.h
  ../common/i18n.h
  ../common/i18n.c
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/servicenames.c
  ../common/servicenames.h
  ../common/tt-version.h
  ../common/ttcommon.h
  ../common/tt_res.h
  ../common/codeconv.h
  ../common/compat_win.h
  ../common/dllutil.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
  )


if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} /DELAYLOAD:setupapi.dll /DELAYLOAD:user32.dll")
endif()

target_compile_definitions(
  ttpcmn
  PRIVATE -DTTPCMN_DLL
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../common
  ../ttpfile
  .
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ole32
  setupapi
  delayimp
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
