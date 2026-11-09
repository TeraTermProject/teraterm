project("matcher")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PROJECT_NAME}
  matcher.c
  test.c
  )

if(MSVC)
  target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    _CRT_SECURE_NO_DEPRECATE
  )
endif()

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    layer_for_unicode
    )
endif()
