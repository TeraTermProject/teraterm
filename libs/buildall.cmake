# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=Win32 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=x64   -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=ARM64 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64   -P buildall.cmake
# .\cmake-3.11.4-win32-x86\bin\cmake.exe -DCMAKE_GENERATOR="Visual Studio 8 2005" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=x86_64 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="NMake Makefiles" -DBUILD_SSL_LIBRARY=OFF -P buildall.cmake

set(BUILD_OPENSSL1 OFF)
set(BUILD_OPENSSL3 OFF)

if("${CMAKE_GENERATOR}" STREQUAL "")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "CYGWIN")
    message(FATAL_ERROR "cmake command is from cygwin")
  endif()
endif()

if((${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005") OR
    (${CMAKE_GENERATOR} MATCHES "Visual Studio 9 2008"))
  set(BUILD_SSL_LIBRARY OFF)
  set(ARCHITECTURE Win32)
endif()

if(NOT DEFINED ARCHITECTURE)
  set(ARCHITECTURE Win32)
  #message(FATAL_ERROR "check ARCHITECTURE")
endif()

if(NOT DEFINED BUILD_SSL_LIBRARY)
  set(BUILD_SSL_LIBRARY ON)
endif()

set(ARCHITECTURE_OPTION -DARCHITECTURE=${ARCHITECTURE})

# install tools
include(${CMAKE_CURRENT_LIST_DIR}/../buildtools/checkperl.cmake)
message("perl=${PERL}")

# ライブラリを1つビルドする, 失敗したら停止する
#   NAME    表示用のライブラリ名
#   ARGN    cmake に渡す引数
function(build_library NAME)
  message("${NAME}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${ARGN}
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "${NAME} build fail ${rv}")
  endif()
endfunction()

set(GENERATOR_OPTION
  -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION})

# build
build_library("oniguruma" ${GENERATOR_OPTION} -P buildoniguruma.cmake)
build_library("zlib" ${GENERATOR_OPTION} -P buildzlib.cmake)
build_library("SFMT" ${GENERATOR_OPTION} -P buildsfmt.cmake)
if(BUILD_SSL_LIBRARY)
  if(BUILD_OPENSSL1)
    build_library("openssl 1.1" ${GENERATOR_OPTION} -P openssl11.cmake)
  endif(BUILD_OPENSSL1)
  if(BUILD_OPENSSL3)
    build_library("openssl 3" ${GENERATOR_OPTION} -P openssl3.cmake)
  endif(BUILD_OPENSSL3)
endif(BUILD_SSL_LIBRARY)
build_library("cJSON" -P buildcjson.cmake)
build_library("argon2" -P buildargon2.cmake)
if(BUILD_SSL_LIBRARY)
  build_library("libressl" ${GENERATOR_OPTION} -P buildlibressl.cmake)
endif(BUILD_SSL_LIBRARY)

message("done buildall.cmake")
