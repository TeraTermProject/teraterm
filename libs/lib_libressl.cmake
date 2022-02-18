
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(LIBRESSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/libressl_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(LIBRESSL_ROOT "${LIBRESSL_ROOT}_x64")
endif()

if(MINGW)
  set(LIBRESSL_INCLUDE_DIRS
    ${LIBRESSL_ROOT}/include
    )

  set(LIBRESSL_LIB
    ${LIBRESSL_ROOT}/lib/libcrypto-47.a
    bcrypt
  )
else()
  set(LIBRESSL_INCLUDE_DIRS
    ${LIBRESSL_ROOT}/include
    )

  set(LIBRESSL_LIB
    debug ${LIBRESSL_ROOT}/lib/crypto-47d.lib
    optimized ${LIBRESSL_ROOT}/lib/crypto-47.lib
    )
endif()
