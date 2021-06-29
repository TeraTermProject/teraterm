set(PACKAGE_NAME "ttpset")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  ttset.c
#  ttset_keyboard.c
  ttset_keyboard_entry.c
  ttpset-version.rc
  ttpset.def
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
      set(CMAKE_C_STANDARD_LIBRARIES "")
      set(CMAKE_CXX_STANDARD_LIBRARIES "")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
    else()
      target_link_options(
        ${PACKAGE_NAME}
        PRIVATE
        /NODEFAULTLIB
        )
    endif()
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../common/tttypes.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/servicenames.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

if(MINGW)
  set_target_properties(
    ${PACKAGE_NAME}
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    layer_for_unicode
#    ${CRT_LIB}
    )
endif()

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
