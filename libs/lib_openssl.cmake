
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

if(MINGW)
  set(OPENSSL_INCLUDE_DIRS
	${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}/include
	)

  set(OPENSSL_LIB
	${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}/lib/libeay32.lib
  )

else()

  set(OPENSSL_INCLUDE_DIRS
	"$<$<CONFIG:Debug>:${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}_debug/include>"
	"$<$<CONFIG:Release>:${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}/include>"
	)

  set(OPENSSL_LIB
	debug ${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}_debug/lib/libeay32.lib
	optimized ${CMAKE_CURRENT_LIST_DIR}/openssl_${TOOLSET}/lib/libeay32.lib
	)

endif()
