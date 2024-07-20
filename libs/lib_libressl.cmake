
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

if(MSVC)
  set(LIBRESSL_ROOT "${CMAKE_CURRENT_LIST_DIR}/libressl_vs_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_GENERATOR_PLATFORM}")
else()
  set(LIBRESSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/libressl_${TOOLSET})
endif()

set(LIBRESSL_INCLUDE_DIRS ${LIBRESSL_ROOT}/include)
if(MINGW)
  set(LIBRESSL_LIB
    ${LIBRESSL_ROOT}/lib/libcrypto.a
  )
else()
  if(IS_MULTI_CONFIG)
    set(LIBRESSL_LIB
      debug ${LIBRESSL_ROOT}/lib/cryptod.lib
      optimized ${LIBRESSL_ROOT}/lib/crypto.lib
    )
  else()
    set(LIBRESSL_LIB ${LIBRESSL_ROOT}/lib/crypto.lib)
  endif()
endif()
