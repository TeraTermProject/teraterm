
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

if(MSVC)
  set(SFMT_ROOT "${CMAKE_CURRENT_LIST_DIR}/SFMT_vs_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_GENERATOR_PLATFORM}")
else()
  set(SFMT_ROOT ${CMAKE_CURRENT_LIST_DIR}/SFMT_${TOOLSET})
endif()

set(SFMT_INCLUDE_DIRS "${SFMT_ROOT}/include")
set(SFMT_LIBRARY_DIRS "${SFMT_ROOT}/lib")

if(MINGW)
  set(SFMT_LIB ${SFMT_LIBRARY_DIRS}/libsfmt.a)
else()
  if(IS_MULTI_CONFIG)
    set(SFMT_LIB
      optimized ${SFMT_LIBRARY_DIRS}/sfmt.lib
      debug ${SFMT_LIBRARY_DIRS}/sfmtd.lib
    )
  else()
    set(SFMT_LIB ${SFMT_LIBRARY_DIRS}/sfmt.lib)
  endif()
endif()
