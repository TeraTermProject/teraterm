
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(LIBRESSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/libressl_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(LIBRESSL_ROOT "${LIBRESSL_ROOT}_x64")
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
