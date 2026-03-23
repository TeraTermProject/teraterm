
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

if(APPLE)
  # On macOS, try Homebrew first, then prebuilt, then system
  find_library(ONIGURUMA_LIB_FOUND onig PATHS /opt/homebrew/lib /usr/local/lib)
  find_path(ONIGURUMA_INC_FOUND oniguruma.h PATHS /opt/homebrew/include /usr/local/include)

  if(ONIGURUMA_LIB_FOUND AND ONIGURUMA_INC_FOUND)
    set(ONIGURUMA_INCLUDE_DIRS ${ONIGURUMA_INC_FOUND})
    set(ONIGURUMA_LIB ${ONIGURUMA_LIB_FOUND})
    message(STATUS "Found oniguruma: ${ONIGURUMA_LIB}")
  else()
    set(ONIGURUMA_ROOT "${CMAKE_CURRENT_LIST_DIR}/oniguruma_${TOOLSET}")
    if(EXISTS "${ONIGURUMA_ROOT}/include")
      set(ONIGURUMA_INCLUDE_DIRS ${ONIGURUMA_ROOT}/include)
      set(ONIGURUMA_LIBRARY_DIRS ${ONIGURUMA_ROOT}/lib)
      set(ONIGURUMA_LIB ${ONIGURUMA_LIBRARY_DIRS}/libonig.a)
    else()
      message(WARNING "Oniguruma not found. Install with: brew install oniguruma")
      set(ONIGURUMA_INCLUDE_DIRS "")
      set(ONIGURUMA_LIB "")
    endif()
  endif()
elseif(MSVC)
  set(ONIGURUMA_ROOT "${CMAKE_CURRENT_LIST_DIR}/oniguruma_vs_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_GENERATOR_PLATFORM}")
  set(ONIGURUMA_INCLUDE_DIRS ${ONIGURUMA_ROOT}/include)
  set(ONIGURUMA_LIBRARY_DIRS ${ONIGURUMA_ROOT}/lib)
  if(IS_MULTI_CONFIG)
    set(ONIGURUMA_LIB
      debug ${ONIGURUMA_LIBRARY_DIRS}/onigd.lib
      optimized ${ONIGURUMA_LIBRARY_DIRS}/onig.lib
    )
  else()
    set(ONIGURUMA_LIB
      ${ONIGURUMA_LIBRARY_DIRS}/onig.lib
    )
  endif()
else()
  set(ONIGURUMA_ROOT ${CMAKE_CURRENT_LIST_DIR}/oniguruma_${TOOLSET})
  set(ONIGURUMA_INCLUDE_DIRS ${ONIGURUMA_ROOT}/include)
  set(ONIGURUMA_LIBRARY_DIRS ${ONIGURUMA_ROOT}/lib)
  set(ONIGURUMA_LIB ${ONIGURUMA_LIBRARY_DIRS}/libonig.a)
endif()
