
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(OPENSSL3_ROOT ${CMAKE_CURRENT_LIST_DIR}/openssl3_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OPENSSL3_ROOT "${OPENSSL3_ROOT}_x64")
endif()

if(MINGW)
  set(OPENSSL3_INCLUDE_DIRS
    ${OPENSSL3_ROOT}/include
    )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENSSL3_LIB
      ${OPENSSL3_ROOT}/lib64/libcrypto.a
      )
  else()
    set(OPENSSL3_LIB
      ${OPENSSL3_ROOT}/lib/libcrypto.a
      )
  endif()
else()
  set(OPENSSL3_INCLUDE_DIRS
    "$<$<CONFIG:Debug>:${OPENSSL3_ROOT}_debug/include>"
    "$<$<CONFIG:Release>:${OPENSSL3_ROOT}/include>"
    )

  set(OPENSSL3_LIB
    debug ${OPENSSL3_ROOT}_debug/lib/libcrypto.lib
    optimized ${OPENSSL3_ROOT}/lib/libcrypto.lib
    )
endif()
