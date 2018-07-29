
string(REPLACE " " "_" GENERATOR_ ${CMAKE_GENERATOR})

set(OPENSSL_INCLUDE_DIRS
  "$<$<CONFIG:Debug>:${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}_debug/include>"
  "$<$<CONFIG:Release>:${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}/include>"
  )

set(OPENSSL_LIB
  debug ${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}_debug/lib/libeay32.lib
  debug ${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}_debug/lib/ssleay32.lib
  optimized ${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}/lib/libeay32.lib
  optimized ${CMAKE_CURRENT_LIST_DIR}/openssl_${GENERATOR_}/lib/ssleay32.lib
  )
