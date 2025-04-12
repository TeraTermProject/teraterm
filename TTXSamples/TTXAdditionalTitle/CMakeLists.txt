project("TTXAdditionalTitle")

add_library(
  ${PROJECT_NAME} SHARED
  TTXAdditionalTitle.c
  ReadMe.txt
  ReadMe-ja.txt
  )

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  ttpcmn
  common_static
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      mingw_dll_tls
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  ../../teraterm/teraterm
  )

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME
  DESTINATION .
)
