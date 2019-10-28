
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(OPENSSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/openssl11_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OPENSSL_ROOT "${OPENSSL_ROOT}_x64")
endif()

if(MINGW)
  set(OPENSSL_INCLUDE_DIRS
	${OPENSSL_ROOT}/include
	)

  set(OPENSSL_LIB
	${OPENSSL_ROOT}/lib/libcrypto.a
  )
else()
  set(OPENSSL_INCLUDE_DIRS
	"$<$<CONFIG:Debug>:${OPENSSL_ROOT}_debug/include>"
	"$<$<CONFIG:Release>:${OPENSSL_ROOT}/include>"
	)

  set(OPENSSL_LIB
	debug ${OPENSSL_ROOT}_debug/lib/libcrypto.lib
	optimized ${OPENSSL_ROOT}/lib/libcrypto.lib
	)
endif()
