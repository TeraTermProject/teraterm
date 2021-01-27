#cmake_minimum_required(VERSION 3.18)
cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "msvcrt")

project(${PACKAGE_NAME})

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
elseif(MINGW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_MINGW_ANSI_STDIO=0")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_WARNING_FLAGS} -g -static -ffunction-sections -fdata-sections ${C_ONLY_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_CXX_WARNING_FLAGS} -g -static -ffunction-sections -fdata-sections")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  endif()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -g")
endif()

add_executable(
  ${PACKAGE_NAME}
  main.c
  outputdebugprintf.cpp
  is.c
  ../../common/asprintf.cpp
  ../../common/compat_win.cpp
  ../../common/layer_for_unicode.cpp
  ../../common/codeconv.cpp
  ../../common/dllutil.cpp
  )

if(MINGW)
  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    ../msvcrt_wrapper.c
    )
endif()

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  ../../common
  )

if(MINGW)
  target_compile_options(
    ${PACKAGE_NAME}
    PUBLIC -Wall
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  gdi32
  )


#target_link_libraries(
#  msvcr70
#  PRIVATE
#  mingw_msvcrt
#  )

if(false)
add_executable(
  crt_snprintf_s
  crt_snprintf_s.cpp
  )

target_compile_options(
  crt_snprintf_s
  PUBLIC -Wall
  )
endif()
