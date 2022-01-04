
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(REBRESSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/libressl_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(REBRESSL_ROOT "${REBRESSL_ROOT}_x64")
endif()

if(MINGW)
  set(LIBRESSL_INCLUDE_DIRS
    ${REBRESSL_ROOT}/include
    )

  set(LIBRESSL_LIB
    ${REBRESSL_ROOT}/lib/libcrypto-47.a
    bcrypt
  )
else()
  set(LIBRESSL_INCLUDE_DIRS
    ${REBRESSL_ROOT}/include
    )

  set(LIBRESSL_LIB
    debug ${REBRESSL_ROOT}/lib/crypto-47d.lib
    optimized ${REBRESSL_ROOT}/lib/crypto-47d.lib
    )
endif()
