cmake_minimum_required(VERSION 3.11)
project(susie_plugin)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /MTd")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI /MTd")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif()
if(NOT DEFINED EXTLIB)
  set(EXTLIB "extlib")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PLUGIN_EXT "sph")
else()
  set(PLUGIN_EXT "spi")
endif()

add_executable(
  spi_tester
  tester/spi_tester.cpp
  libsusieplugin.cpp
  libsusieplugin.h
  )

target_include_directories(
  spi_tester
  PRIVATE
  .
  )

target_compile_definitions(
  spi_tester
  PRIVATE
  WITHOUT_TERATERM
  )

add_library(
  ttspijpeg SHARED
  plugin/spi_jpeg.cpp
  plugin/spi_common.cpp
  plugin/spi_common.h
  plugin/susie.def
  )

set_target_properties(
  ttspijpeg
  PROPERTIES SUFFIX ".${PLUGIN_EXT}"
  )

target_include_directories(
  ttspijpeg
  PRIVATE
  ${EXTLIB}/libjpeg-turbo/include
)

target_link_libraries(
  ttspijpeg
  ${CMAKE_CURRENT_SOURCE_DIR}/${EXTLIB}/libjpeg-turbo/lib/jpeg-static.lib
  )

add_library(
  ttspipng SHARED
  plugin/spi_png.cpp
  plugin/spi_common.cpp
  plugin/spi_common.h
  plugin/susie.def
  )

target_include_directories(
  ttspipng
  PRIVATE
  ${EXTLIB}/libpng/include
)

set_target_properties(
  ttspipng
  PROPERTIES SUFFIX ".${PLUGIN_EXT}"
  )

target_link_libraries(
  ttspipng
  ${CMAKE_CURRENT_SOURCE_DIR}/${EXTLIB}/libpng/lib/libpng16_static.lib
  ${CMAKE_CURRENT_SOURCE_DIR}/${EXTLIB}/zlib/lib/zlibstatic.lib
  )
