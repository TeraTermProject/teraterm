
include(${CMAKE_CURRENT_LIST_DIR}/script_support.cmake)

set(ZLIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/zlib_${TOOLSET})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ZLIB_ROOT "${ZLIB_ROOT}_x64")
endif()

set(ZLIB_INCLUDE_DIRS ${ZLIB_ROOT}/include)
set(ZLIB_LIBRARY_DIRS ${ZLIB_ROOT}/lib)
if(MINGW)
  set(ZLIB_LIB
    ${ZLIB_LIBRARY_DIRS}/libzlibstatic.a
    )
else()
  set(ZLIB_LIB
    debug ${ZLIB_LIBRARY_DIRS}/zlibstaticd.lib
    optimized ${ZLIB_LIBRARY_DIRS}/zlibstatic.lib
    )
endif()
