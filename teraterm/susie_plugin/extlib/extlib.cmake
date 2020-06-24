set(CONFIG "Release")
#set(CONFIG "debug")
#set(GENERATE_OPTION "-G" "Visual Studio 8 2005")
#set(INSTALL_PREFIX_ADD "vs2005_win32/")

# zlib

file(DOWNLOAD
  https://zlib.net/zlib-1.2.11.tar.xz
  ./download/zlib-1.2.11.tar.xz
  SHOW_PROGRESS
  EXPECTED_HASH SHA256=4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066
  )

file(MAKE_DIRECTORY "build/zlib/src")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/download/zlib-1.2.11.tar.xz
  WORKING_DIRECTORY "build/zlib/src")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy patched_file/zlib-1.2.11/CMakeLists.txt build/zlib/src/zlib-1.2.11)
file(MAKE_DIRECTORY "build/zlib/build")
execute_process(
  COMMAND ${CMAKE_COMMAND} ../src/zlib-1.2.11 -DASM686=off -DAMD64=off
  -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/${INSTALL_PREFIX_ADD}zlib ${GENERATE_OPTION}
  WORKING_DIRECTORY "build/zlib/build")
execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config ${CONFIG} --target install
  WORKING_DIRECTORY "build/zlib/build")

# libpng

file(DOWNLOAD
  https://download.sourceforge.net/libpng/libpng-1.6.37.tar.xz
  ./download/libpng-1.6.37.tar.xz
  SHOW_PROGRESS
  EXPECTED_HASH SHA256=505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca
  )

file(MAKE_DIRECTORY "build/libpng/src")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/download/libpng-1.6.37.tar.xz
  WORKING_DIRECTORY "build/libpng/src")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy patched_file/libpng-1.6.37/CMakeLists.txt build/libpng/src/libpng-1.6.37)

file(MAKE_DIRECTORY "build/libpng/build")
execute_process(
  COMMAND ${CMAKE_COMMAND} ../src/libpng-1.6.37 -DPNG_SHARED=off -DPNG_STATIC=on -DPNG_TESTS=off
  -DPNG_DEBUG=off -DPNGARG=off -DPNG_BUILD_ZLIB=off -DZLIB_INCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/zlib/include -DZLIB_LIBRARY=${CMAKE_CURRENT_SOURCE_DIR}/zlib/lib ${GENERATE_OPTION}
  -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/${INSTALL_PREFIX_ADD}libpng
  WORKING_DIRECTORY "build/libpng/build")
execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config ${CONFIG} --target install
  WORKING_DIRECTORY "build/libpng/build")

# libjpeg-turbo

file(DOWNLOAD
  https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.4.tar.gz
  ./download/libjpeg-turbo-2.0.4.tar.gz
  SHOW_PROGRESS
  EXPECTED_HASH SHA256=7777c3c19762940cff42b3ba4d7cd5c52d1671b39a79532050c85efb99079064
  )

file(MAKE_DIRECTORY "build/libjpeg/src")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/download/libjpeg-turbo-2.0.4.tar.gz
  WORKING_DIRECTORY "build/libjpeg/src")

file(MAKE_DIRECTORY "build/libjpeg/build")
execute_process(
  COMMAND ${CMAKE_COMMAND} ../src/libjpeg-turbo-2.0.4 -DENABLE_SHARED=OFF -DWITH_SIMD=OFF
  -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/${INSTALL_PREFIX_ADD}libjpeg-turbo ${GENERATE_OPTION}
  WORKING_DIRECTORY "build/libjpeg/build")

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config ${CONFIG} --target install
  WORKING_DIRECTORY "build/libjpeg/build")

